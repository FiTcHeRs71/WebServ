#ifndef EVENTLOOP_HPP
# define EVENTLOOP_HPP

# include <iostream>
# include <vector>
# include <map>
# include <sys/poll.h>
# include <algorithm>
# include <fcntl.h>
# include <netinet/in.h>
# include <poll.h>
# include <unistd.h>
# include "Connection.hpp"
# include "Config.hpp"
# include "ListenSockets.hpp"

using namespace std;

/**
 * @brief Boucle d'evenements unique du serveur.
 *
 * Un seul poll() surveille tous les fds du processus : sockets d'ecoute,
 * connexions clients, et plus tard les pipes CGI. C'est le seul endroit
 * ou une I/O peut etre declenchee. Le dispatch se fait par appartenance
 * du fd a _ListenFds (accept) ou a _Clients (HandleClientEvent).
 */
class EventLoop
{
	private:

	vector<struct pollfd>	_Pollfds;		///< Le tableau passe a poll(), index volatile
	map<int, Connection>	_Clients;		///< fd client -> etat de la connexion
	map<int, size_t>		_ListenFds;		///< fd d'ecoute -> index dans _AddrPorts
	const ConfigParser		*_Config;		///< Conf parse, pour retrouver le TAddrPortGroup d'un client
	bool					_Running;		///< Garde Run() en vie tant qu'il est true
	map<int, int>			_CgiToClient;	///< fd de pipe -> fd du client qui attend la reponse
	vector<int>				_toClose;		///< vecteur contenant les fd cleint a fermer
	

	public:

	EventLoop(const ConfigParser &config, const ListenSockets &sockets);	///< Enregistre les listen-fds dans poll
	~EventLoop(void);
	EventLoop(const EventLoop& to_copy);
	EventLoop &operator=(const EventLoop& src);

	void	Run(void);							///< Boucle jusqu'a _Running == false
	void	AddFd(int fd, short events);		///< Utilise aussi par B-07 pour les pipes CGI
	void	RemoveFd(int fd);					///< Retire fd de _Pollfds, n'invalide pas _Clients
	void	SetEvents(int fd, short events);	///< Re-arme POLLIN / POLLOUT sur un fd deja surveille
	void	RegisterCgi(int client_fd, int cgi_read_fd, int cgi_write_fd);	///< Branche les pipes CGI sur le poll (B-07)
	void	UnregisterCgi(int client_fd);		///< Ferme et oublie les pipes d'un CGI fini (B-07)
	void	CloseConnection(int fd);

	private:

	void	AcceptNewClients(int listen_fd);		///< accept() sur un listen-fd pret, O_NONBLOCK immediat
	bool	HandleClientEvent(size_t index);		///< I/O client (B-03) : recv/send selon revents
	int		FindFd(int fd);							///< Index de fd dans _Pollfds, -1 si absent
	void	HandleCgiEvent(int fd, short revents);	///< I/O pipe CGI (B-07) : lit/ecrit selon revents
	int		ComputeTimeout(void) const;				///< millisecondes a passer a poll()
	void	SweepTimeouts(void);					///< appele a CHAQUE retour de poll(), meme sur 0
	void	Shutdown(void);							///< Ferme tous les fds client avant de rendre la main
};

#endif
