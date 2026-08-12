#ifndef EVENTLOOP_HPP
# define EVENTLOOP_HPP

# include <iostream>
# include <vector>
# include <map>
# include <sys/poll.h>
# include "Connection.hpp"
# include "Config.hpp"
# include "ListenSockets.hpp"

using namespace std;

class EventLoop
{
	private:

	std::vector<struct pollfd>	_Pollfds;		///< Le tableau passe a poll(), index volatile
	std::map<int, Connection>	_Clients;		///< fd client -> etat de la connexion
	std::map<int, size_t>		_ListenFds;		///< fd d'ecoute -> index dans _AddrPorts
	const ConfigParser			*_Config;
	bool						_Running;

	public:

	EventLoop(const ConfigParser &config, const ListenSockets &sockets);
	~EventLoop(void);
	EventLoop(const EventLoop& to_copy);
	EventLoop &operator=(const EventLoop& src);

	void	Run(void);						///< Boucle jusqu'a _Running == false
	void	AddFd(int fd, short events);	///< Utilise aussi par B-07 pour les pipes CGI
	void	RemoveFd(int fd);
	void	SetEvents(int fd, short events);

	private:

	void	AcceptNewClients(int listen_fd);
	void	HandleClientEvent(size_t index);
};

#endif