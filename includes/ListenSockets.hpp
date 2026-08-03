#ifndef LISTEN_SOCKETS_HPP
# define LISTEN_SOCKETS_HPP

# include "ServerConfig.hpp"
# include "Config.hpp"
# include <iostream>
# include <vector>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>
# include <arpa/inet.h>
# include <string.h>
# include <sstream>

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
	vector<int> _sockFd;

	protected:



	public:

	/*===Canonical Form===*/
	ListenSockets(const std::vector<ServerConfig> &servers);
	~ListenSockets(void);
	ListenSockets(const ListenSockets& to_copy);
	ListenSockets &operator=(const ListenSockets& src);
	/*===Getters & Setters===*/


	/*===Member Function===*/
	bool	creatSocket(const pair<string, int>& listen);
};

#endif /*LISTEN_SOCKETS_HPP*/
