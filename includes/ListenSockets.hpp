#ifndef LISTEN_SOCKETS_HPP
# define LISTEN_SOCKETS_HPP

# include "Config.hpp"
# include <iostream>
# include <string>
# include <unistd.h>
# include <vector>
# include <sys/types.h>
# include <sys/socket.h>
# include <netdb.h>
# include <string.h>
# include <sstream>
# include <fcntl.h>

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
	vector<int>	_ServFd;

	public:

	/*===Canonical Form===*/
	ListenSockets(const vector<TAddrPortGroup>& AddrPorts);
	~ListenSockets(void);
	ListenSockets(const ListenSockets& to_copy);
	ListenSockets &operator=(const ListenSockets& src);

	/*===Getters & Setters===*/
	const vector<int>& getServFd() const;

	/*===Member Function===*/
	bool	creatSocket(const string& Host, const int& Port);
	void	closeFd();

	friend ostream& operator<<(ostream& flux, const ListenSockets& listen);
};

#endif
