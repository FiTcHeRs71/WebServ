#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include "LocationConfig.hpp"
# include <iostream>
# include <map>
# include <ostream>
# include <string>
# include <vector>

using namespace std;

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

/**
 * @brief Represente la configuration d'un serveur virtuel (bloc "server").
 *
 * Regroupe les hosts/ports ecoutes ainsi que l'ensemble des
 * LocationConfig associees, et permet de resoudre la location
 * correspondant a une requete donnee.
 */
class ServerConfig
{
	friend class				ConfigParser;

	private:

	vector<TListenConfig>	_Listens;
	vector<string>			_ServerNames;
	map<int, string>		_ErrorPages;
	vector<LocationConfig>	_Locations;
	size_t					_ClientMaxBodySize;
	bool					_HasClientMaxBodySize;

	protected:



	public:

	/*===Canonical Form===*/
	ServerConfig(void);
	~ServerConfig(void);
	ServerConfig(const ServerConfig& to_copy);
	ServerConfig &operator=(const ServerConfig& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/
	const LocationConfig	*Resolve(const std::string &uri)const;	///< TODO : location matchant une requete
	string					build_path(const LocationConfig &location, const string &uri)const;

	/*===Friends===*/
	friend ostream			&operator<<(ostream &flux, const ServerConfig &src);
};

/*=== HELPERS ===*/
bool	is_segment_boundary(const string &uri, const string &path);
bool	is_all_digits(const string &s);
bool	is_valid_ipv4(const string &host);
bool	operator==(const TListenConfig &Listen_a, const TListenConfig &Listen_b);

#endif /*SERVER_CONFIG_HPP*/