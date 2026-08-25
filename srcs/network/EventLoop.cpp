#include "../../includes/EventLoop.hpp"
#include <cstddef>
#include <arpa/inet.h>
#include <ctime>
#include <sys/poll.h>

/**
 * @brief Enregistre les sockets d'ecoute dans poll et dans _ListenFds
 *
 * Un TAddrPortGroup = un listen-fd, dans le meme ordre que getServFd().
 * La valeur stockee est l'index du groupe dans _AddrPorts, pour que chaque
 * client accepte plus tard sache quel ServerConfig peut lui repondre.
 *
 * @param config Le parser deja rempli par parse(), conserve par pointeur.
 * @param sockets Les listen-fds deja bind/listen/O_NONBLOCK (B-01).
 */
EventLoop::EventLoop(const ConfigParser &config, const ListenSockets &sockets)
	:_Config(&config)
	,_Running(true)
{
	vector<int>				servFds = sockets.getServFd();

	for (size_t i = 0; i <servFds.size(); i++)
	{
		_ListenFds[servFds[i]] = i; ///< servFds[i] is the socket, i is the index in _AddrPorts
		AddFd(servFds[i], POLLIN);
	}

	//cout << "EventLoop constructor called." << endl;
}

EventLoop::~EventLoop(void)
{
	//cout << "EventLoop destructor called." << endl;
}

EventLoop::EventLoop(const EventLoop& to_copy)
	:_Pollfds(to_copy._Pollfds)
	,_Clients(to_copy._Clients)
	,_ListenFds(to_copy._ListenFds)
	,_Config(to_copy._Config)
	,_Running(to_copy._Running)
	,_CgiToClient(to_copy._CgiToClient)
{
	//cout << "EventLoop copy constructor called." << endl;
}

EventLoop &EventLoop::operator=(const EventLoop& src)
{
	if (this != &src)
	{
		this->_Pollfds = src._Pollfds;
		this->_Clients = src._Clients;
		this->_ListenFds = src._ListenFds;
		this->_Config = src._Config;
		this->_Running = src._Running;
		this->_CgiToClient = src._CgiToClient;
	}
	//cout << "EventLoop copy assignment operator called." << endl;
	return (*this);
}

/**
 * @brief Boucle unique : poll() puis dispatch listen-fd / client-fd
 *
 * @return void
 */
void	EventLoop::Run(void)
{
	int	status;
	while (_Running)
	{
		status = poll(&_Pollfds[0], _Pollfds.size(), ComputeTimeout());
		SweepTimeouts();
		if (status == 0)
		{
			for (size_t i = 0; i < _toClose.size(); i++)
				CloseConnection(_toClose[i]);
			_toClose.clear();
			continue ;
		}
		else if (status < 0)
		{
			if (errno == EINTR)
							continue ;
			cerr << "Error: poll() failed." << endl;
			break ;			//TODO: A check break ou throw ?
		}
		for (size_t i = 0; i < _Pollfds.size(); i++)
		{
			if (_Pollfds[i].revents == 0)
				continue ;
			int	fd = _Pollfds[i].fd;
			if (_ListenFds.count(fd)) ///< 1 is a listen fd so we accept, 0 isn't, its a client so we handle
			{
				if (_Pollfds[i].revents & POLLIN)
					AcceptNewClients(fd);
				continue;
			}
			else if (_CgiToClient.count(fd))
				HandleCgiEvent(fd, _Pollfds[i].revents);
			else if (!HandleClientEvent(i))
				_toClose.push_back(_Pollfds[i].fd);
		}
		for (size_t i = 0; i < _toClose.size(); i++)
			CloseConnection(_toClose[i]);
		_toClose.clear();
	}
}

/**
 * @brief Cherche un fd dans _Pollfds
 *
 * @param fd Le descripteur a localiser.
 * @return Son index dans _Pollfds, ou -1 s'il n'y est pas.
 */
int	EventLoop::FindFd(int fd)
{
	for (size_t i = 0; i < _Pollfds.size(); i++)
	{
		if (_Pollfds[i].fd == fd)
			return (static_cast<int>(i));
	}
	return (-1);
}

/**
 * @brief Ajoute un fd a _Pollfds, ou met a jour ses events s'il y est deja
 *
 * Sert aussi a B-07 pour enregistrer les pipes CGI dans le meme poll.
 *
 * @param fd Le descripteur a surveiller. Ignore si fd < 0.
 * @param events Masque poll (POLLIN, POLLOUT, ou les deux).
 * @return void
 */
void	EventLoop::AddFd(int fd, short events)
{
	struct pollfd	newFd;
	int				idx;

	if (fd < 0)
		return ;
	idx = FindFd(fd);
	if (idx != -1)
	{
		_Pollfds[idx].events = events;
		return ;
	}
	newFd.fd = fd;
	newFd.events = events;
	newFd.revents = 0;
	_Pollfds.push_back(newFd);
}

/**
 * @brief Retire un fd de _Pollfds
 *
 * Invalide les index : ne pas appeler pendant l'iteration de Run(),
 * collecter les fds a fermer et purger apres la boucle.
 *
 * @param fd Le descripteur a retirer. Ignore si fd < 0.
 * @return void
 */
void	EventLoop::RemoveFd(int fd)
{
	int	idx;

	if (fd < 0)
		return ;
	idx = FindFd(fd);
	if (idx != -1)
	{
		_Pollfds.erase(_Pollfds.begin() + idx);
		return ;
	}
	cerr << "This fd wasnt found." << endl; //checker si message d'erreur necessaire ou si on skip.
}

/**
 * @brief Change le masque d'events d'un fd deja present dans _Pollfds
 *
 * POLLOUT ne doit etre arme que si le client a des octets a envoyer (B-03).
 *
 * @param fd Le descripteur a modifier. Ignore si fd < 0.
 * @param events Nouveau masque poll.
 * @return void
 */
void	EventLoop::SetEvents(int fd, short events)
{
	int				idx;

	if (fd < 0)
		return ;
	idx = FindFd(fd);
	if (idx != -1)
	{
		_Pollfds[idx].events = events;
		return ;
	}
	cerr << "This fd wasnt found." << endl; //checker si message d'erreur necessaire ou si on skip.
}

/**
 * @brief Accepte un client sur un socket d'ecoute pret
 *
 * A appeler seulement si poll() a signale POLLIN sur listen_fd.
 * Le nouveau fd est passe en O_NONBLOCK tout de suite (le flag du
 * listen-fd n'est pas herite), puis range dans _Clients et _Pollfds.
 * L'index du TAddrPortGroup se lit dans _ListenFds[listen_fd] pour
 * le coller sur la Connection (choix du ServerConfig par le module C).
 *
 * @param listen_fd Le socket d'ecoute signale pret par poll().
 * @return void
 */
void	EventLoop::AcceptNewClients(int listen_fd)
{
	struct sockaddr_in	clientAddr;
	socklen_t			clientLen = sizeof(clientAddr);

	if (listen_fd <= 0)
		return ;
	int clientFd = accept(listen_fd, (struct sockaddr *)&clientAddr, &clientLen);
	if (clientFd < 0)
	{
		cerr << "Error: accept() failure." << endl;
		return ;
	}
	if (fcntl(clientFd, F_SETFL, O_NONBLOCK) < 0){
		close(clientFd);
		return;
	}
	size_t groupIndex = _ListenFds[listen_fd];
	_Clients[clientFd] = Connection(clientFd, groupIndex, _Config);
	_Clients[clientFd].setLastActivity();
	AddFd(clientFd, POLLIN);
	_Clients[clientFd].setIpV4(inet_ntoa(clientAddr.sin_addr));
}


/**
 * @brief Traite un evenement sur un fd qui n'est pas un listen-fd
 *
 * B-03 : POLLIN -> recv dans inBuf, POLLOUT -> send depuis outBuf.
 * B-04 : POLLHUP / POLLERR / recv == 0 -> close + RemoveFd.
 *
 * @param index Index du pollfd client dans _Pollfds.
 * @return true when all is ok
 * @return false when the fd need to be closed
 */
bool	EventLoop::HandleClientEvent(size_t index)
{
	int		fd = _Pollfds[index].fd;
	short	revents = _Pollfds[index].revents;

	map<int, Connection>::iterator it = _Clients.find(fd);
	if (it == _Clients.end())
		return true;
	if (revents & (POLLERR | POLLNVAL))
		return (false);
	if (_Pollfds[index].revents & POLLIN){
		it->second.OnReadable();
		if (it->second.HasPendingOutput())
			SetEvents(fd, POLLIN | POLLOUT);
	}
	if (_Pollfds[index].revents & POLLOUT){
		it->second.OnWritable();
		if (!it->second.HasPendingOutput())
			SetEvents(fd, POLLIN);
	}
	if (revents & POLLHUP)
		return (false);
	if (it->second.getState() == CONN_CLOSING && !it->second.HasPendingOutput())
		return false;
	return true;
}

/**
 * @brief Calcule le timeout (ms) a passer a poll().*
 * Parcourt _Clients : requete incomplete (CONN_READING, !_ReqComplete)
 * -> TIMEOUT_HEADER ; keep-alive inactif (CONN_READING, _ReqComplete)
 * -> TIMEOUT_IDLE. Renvoie le plus petit restant, borne dans [0, 1000].
 * -1 seulement s'il n'y a aucun client (poll peut dormir).*
 * @return Millisecondes avant le prochain reveil, ou -1 si _Clients est vide.
*/
int	EventLoop::ComputeTimeout(void) const
{
	if (_Clients.empty())
		return(-1);
	time_t	next_up = 100000;
	time_t	remain;
	for(map<int, Connection>::const_iterator it = _Clients.begin(); it != _Clients.end(); it++)
	{
		if (!it->second.getReqComplete() && it->second.getState() == CONN_READING)
		{
			remain = (TIMEOUT_HEADER - (time(NULL) - it->second.getLastActivity())) * 1000;
			if (remain < next_up)
				next_up = remain;
		}
		else if(it->second.getReqComplete() && it->second.getState() == CONN_READING)
		{
			remain = (TIMEOUT_IDLE - (time(NULL) - it->second.getLastActivity())) * 1000;
			if (remain < next_up)
				next_up = remain;
		}
	}
	if (next_up < 0)
		return(0);
	else if (next_up < 1000)
		return(static_cast<int>(next_up));
	else
		return(1000);
}

/**
 * @brief Point de sortie unique d'une connexion client (B-04 / B-05).*
 * Retire fd de _Pollfds et de _Clients, puis close(fd). erase par cle :
 * no-op si le fd n'est plus dans la map. Ne pas appeler pendant l'iteration
 * de SweepTimeouts() sans avancer l'iterateur avant.*
 * @param fd Le descripteur client a fermer.
 * @return void
*/
void	EventLoop::CloseConnection(int fd)
{
	RemoveFd(fd);
	_Clients.erase(fd);
	if (close(fd) < 0)
		cerr << "Error: close() failed on the clients fd." << endl;
}

/**
 * @brief Expire les connexions inactives. A appeler a chaque retour de poll(),
 * y compris quand poll() rend 0.*
 * Timeout lecture : SendErrorAndClose(408) puis POLLOUT (la reponse doit partir
 * avant CloseConnection). Timeout idle : fd pousse dans _toClose, close
 * silencieux, pas de 408.*
 * @return void
*/
void	EventLoop::SweepTimeouts(void)
{
	if(_Clients.empty())
		return ;
	for(map<int, Connection>::iterator it = _Clients.begin(); it != _Clients.end(); it++)
	{
		time_t	toClose = 1;
		if (!it->second.getReqComplete() && it->second.getState() == CONN_READING)
			toClose = (TIMEOUT_HEADER - (time(NULL) - it->second.getLastActivity())) * 1000;
		else if(it->second.getReqComplete() && it->second.getState() == CONN_READING)
			toClose = (TIMEOUT_IDLE - (time(NULL) - it->second.getLastActivity())) * 1000;
		if (toClose <= 0)
		{
			if(!it->second.getReqComplete())
			{
				it->second.SendErrorAndClose(408);
				SetEvents(it->first, POLLOUT);
			}
			else
				_toClose.push_back(it->first);
		}
	}
}

/**
 * @brief Branche les pipes d'un CGI fraichement forke sur le poll (STUB B-07)
 *
 * Ajoute les deux pipes a _Pollfds et les associe au client dans
 * _CgiToClient, pour que HandleCgiEvent() sache a qui rendre la reponse.
 *
 * @param client_fd Le client qui attend la sortie du script.
 * @param cgi_read_fd Pipe de lecture (sortie du script), surveille en POLLIN.
 * @param cgi_write_fd Pipe d'ecriture (corps de la requete), POLLOUT. -1 si rien a envoyer.
 * @return void
 */
void	EventLoop::RegisterCgi(int client_fd, int cgi_read_fd, int cgi_write_fd)
{
	(void)client_fd;
	(void)cgi_read_fd;
	(void)cgi_write_fd;
}

/**
 * @brief Debranche les pipes d'un CGI termine ou abandonne (STUB B-07)
 *
 * A appeler a la fin du script comme a la deconnexion prematuree du client.
 * Sans ca, un fd recycle par le noyau retomberait dans la branche CGI du
 * dispatch. RemoveFd() invalide les index : purger apres la boucle de Run().
 *
 * @param client_fd Le client dont il faut fermer les pipes.
 * @return void
 */
void	EventLoop::UnregisterCgi(int client_fd)
{
	(void)client_fd;
}

/**
 * @brief Traite un evenement sur un pipe CGI (STUB B-07)
 *
 * Remonte au client via _CgiToClient : POLLIN -> lire la sortie du script
 * vers son outBuf, POLLOUT -> pousser le corps de la requete dans le script.
 * POLLHUP sur le pipe de lecture = script fini, la reponse est complete.
 *
 * @param fd Le pipe signale pret par poll().
 * @param revents Masque retourne par poll() pour ce pipe.
 * @return void
 */
void	EventLoop::HandleCgiEvent(int fd, short revents)
{
	(void)fd;
	(void)revents;
}
