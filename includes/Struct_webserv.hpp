#ifndef STRUCT_WEBSERV_HPP
# define STRUCT_WEBSERV_HPP

#include <cstddef>
# include <iostream>
# include <vector>

using namespace std;

/**
 * @brief Represente un socket d'ecoute et les serveurs qui se le partagent
 *
 * Le fichier de conf donne une vue "par serveur" (chaque ServerConfig porte ses
 * TListenConfig). Un TAddrPortGroup est la vue inverse, "par couple Host:Port" :
 * on ne peut faire qu'un seul bind() sur une adresse:port donnee, alors que
 * plusieurs blocs server peuvent declarer le meme listen. build_addr_port_groups()
 * fait donc ce regroupement, et un groupe = exactement un socket.
 *
 * Host / Port  : les arguments du bind().
 * ServerIndexes: les indexes dans _Servers des blocs server joignables sur ce
 *                socket, dans l'ordre du fichier. C'est parmi eux que le routage
 *                cherchera une correspondance avec le header Host: de la requete.
 * DefaultIndex : le serveur a utiliser quand aucun server_name ne matche (ou que
 *                le header Host: est absent). Initialise a _Servers.size(), un
 *                index volontairement invalide qui sert de sentinelle "aucun
 *                default_server explicite vu pour l'instant" : il permet de
 *                detecter un second default_server sur le meme groupe (erreur),
 *                puis, s'il est toujours a la sentinelle en fin de passe, d'appliquer
 *                la regle NGINX : le premier serveur declare devient le default.
 *
 * ex : deux server { listen 8080; } produisent un seul TAddrPortGroup
 *      { "0.0.0.0", 8080, [0, 1], 0 }
 */
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
