#ifndef STRUCT_WEBSERV_HPP
# define STRUCT_WEBSERV_HPP

#include <cstddef>
# include <iostream>
# include <vector>

using namespace std;

struct	TAddrPortGroup
{
	string			Host;
	int				Port;
	vector<size_t>	ServerIndexes;
	size_t			DefaultIndex;

	TAddrPortGroup(void);
	TAddrPortGroup(const string &host, const int port, const size_t default_index);
};

/**
 * @brief Represente un point decoute issu d'une directive <listen>
 * 
 * Un ListenConfig porte toujours une IPv4 litterale : "*" est normalise en
 * "0.0.0.0" des le parsing et un hostname esr resolu via getaddrinfo.
 * Une seule directive Listen peut donc produire plusieurs ListenConfig si le 
 * hostname resout vers plusieurs adresses (Comportement NGINX).

 * ex : ->listen<- 127.0.0.1:8080 default_server;
 */
struct	TListenConfig
{
	string	Host;
	int		Port;
	bool	IsDefaultServer;

	TListenConfig(void);
	TListenConfig(const string &host, int port, bool is_default_server);
};

#endif /*STRUCT_WEBSERV_HPP*/