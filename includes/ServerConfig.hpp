#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include "LocationConfig.hpp"
# include <iostream>
# include <map>
# include <ostream>
# include <string>
# include <utility>
# include <vector>

using namespace std;

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

	vector<pair<string, int> >	_Listens;
	vector<string>				_ServerNames;
	map<int, string>			_ErrorPages;
	vector<LocationConfig>		_Locations;
	size_t						_ClientMaxBodySize;

	protected:



	public:

	/*===Canonical Form===*/
	ServerConfig(void);
	~ServerConfig(void);
	ServerConfig(const ServerConfig& to_copy);
	ServerConfig &operator=(const ServerConfig& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/
	const LocationConfig	*Resolve(const std::string &host, int port, const std::string &uri);


	/*===Friends===*/
	friend ostream			&operator<<(ostream &flux, const ServerConfig &src);
};

#endif /*SERVER_CONFIG_HPP*/