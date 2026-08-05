#ifndef LISTEN_SOCKETS_HPP
# define LISTEN_SOCKETS_HPP

# include "ServerConfig.hpp"
# include "Config.hpp"
# include <iostream>
# include <unistd.h>
# include <vector>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <string.h>
# include <sstream>
# include <fcntl.h>

# define BACKLOG 511
using namespace std;

/**
 * @brief Gere l'ensemble des sockets d'ecoute du serveur.
 *
 * Cree et configure un socket en ecoute pour chaque couple
 * host/port defini par les ServerConfig fournies.
 */
class ListenSockets
{
	private:
	vector<pair<int, string> > _ServFd;

	public:

	/*===Canonical Form===*/
	ListenSockets(const std::vector<ServerConfig> &servers);
	~ListenSockets(void);
	ListenSockets(const ListenSockets& to_copy);
	ListenSockets &operator=(const ListenSockets& src);

	/*===Getters & Setters===*/
	vector<pair<int, string> > getServFd() const;

	/*===Member Function===*/
	bool	creatSocket(const pair<string, int>& listen);
	bool	findHostPort(string hostPort);
	void	closeFd();
	friend ostream& operator<<(ostream& flux, const ListenSockets& listen);
};

#endif
