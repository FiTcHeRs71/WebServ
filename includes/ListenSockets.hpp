#ifndef LISTEN_SOCKETS_HPP
# define LISTEN_SOCKETS_HPP

# include "ServerConfig.hpp"
# include <iostream>
# include <vector>

/**
 * @brief Gere l'ensemble des sockets d'ecoute du serveur.
 *
 * Cree et configure un socket en ecoute pour chaque couple
 * host/port defini par les ServerConfig fournies.
 */
class ListenSockets
{
	private:



	protected:



	public:

	/*===Canonical Form===*/
	ListenSockets(const std::vector<ServerConfig> &servers);
	~ListenSockets(void);
	ListenSockets(const ListenSockets& to_copy);
	ListenSockets &operator=(const ListenSockets& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/

};

#endif /*LISTEN_SOCKETS_HPP*/