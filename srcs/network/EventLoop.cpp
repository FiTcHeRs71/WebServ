#include "../../includes/EventLoop.hpp"
#include <cstddef>
#include <arpa/inet.h>


// extern int _Signal = 0;

// void	_SignalHandler(int sig){
// 	_Signal = sig;
// }



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
	}
	//cout << "EventLoop copy assignment operator called." << endl;
	return (*this);
}

/**
 * @brief Boucle unique : poll() puis dispatch listen-fd / client-fd
 *
 * timeout = -1 tant que B-05 n'a pas de ComputeTimeout(). Un retour 0
 * (timeout) est ignore. Un retour < 0 avec EINTR relance la boucle ;
 * toute autre erreur arrete Run().
 *
 * @return void
 */
void	EventLoop::Run(void)
{
	// signal(SIGINT, _SignalHandler);
	int	status;
	while (_Running)
	{
		status = poll(&_Pollfds[0], _Pollfds.size(), -1);
		if (status == 0)
			continue ;
		else if (status < 0)
		{
			if (errno == EINTR) ///< errno is okay, subject says forbidden only after a read or write
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
			else
				HandleClientEvent(i);
		}
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
	_Clients[clientFd] = Connection(clientFd, groupIndex);
	AddFd(clientFd, POLLIN);
	_Clients[clientFd].setIpV4(inet_ntoa(clientAddr.sin_addr));
	// TODO:	needs to be finished when connection is done and we can add the connection state
	//			should have these lines then :	size_t groupIndex = _ListenFds[listen_fd];
	//											_Clients[clientFd] = Connection(clientFd, groupIndex);
}


/**
 * @brief Traite un evenement sur un fd qui n'est pas un listen-fd
 *
 * B-03 : POLLIN -> recv dans inBuf, POLLOUT -> send depuis outBuf.
 * B-04 : POLLHUP / POLLERR / recv == 0 -> close + RemoveFd.
 *
 * @param index Index du pollfd client dans _Pollfds.
 * @return void
 */
void	EventLoop::HandleClientEvent(size_t index)
{
	int fd = _Pollfds[index].fd;
	map<int, Connection>::iterator it = _Clients.find(fd);
	if (it == _Clients.end())
		return;
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
	if (it->second.getState() == CONN_CLOSING){
		if (close(fd) < 0)
			cerr << "Error: " << strerror(errno) << endl;
		RemoveFd(fd);
	}
	//TODO Interception des POLLHUP POLLERR POLLNVAL et si recv == 0 pour ticket (B-04)
}

