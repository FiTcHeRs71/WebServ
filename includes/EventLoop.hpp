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

	std::vector<struct pollfd>	_Pollfds;		///< Le tableau passe a poll(), index volatile
	std::map<int, Connection>	_Clients;		///< fd client -> etat de la connexion
	std::map<int, size_t>		_ListenFds;		///< fd d'ecoute -> index dans _AddrPorts
	const ConfigParser			*_Config;		///< Conf parse, pour retrouver le TAddrPortGroup d'un client
	bool						_Running;		///< Garde Run() en vie tant qu'il est true

	public:

	EventLoop(const ConfigParser &config, const ListenSockets &sockets);	///< Enregistre les listen-fds dans poll
	~EventLoop(void);
	EventLoop(const EventLoop& to_copy);
	EventLoop &operator=(const EventLoop& src);

	void	Run(void);							///< Boucle jusqu'a _Running == false
	void	AddFd(int fd, short events);		///< Utilise aussi par B-07 pour les pipes CGI
	void	RemoveFd(int fd);					///< Retire fd de _Pollfds, n'invalide pas _Clients
	void	SetEvents(int fd, short events);	///< Re-arme POLLIN / POLLOUT sur un fd deja surveille

	private:

	void	AcceptNewClients(int listen_fd);	///< accept() sur un listen-fd pret, O_NONBLOCK immediat
	void	HandleClientEvent(size_t index);	///< I/O client (B-03) : recv/send selon revents
	int		FindFd(int fd);						///< Index de fd dans _Pollfds, -1 si absent

};

#endif