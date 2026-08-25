#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include "Struct_webserv.hpp"
# include "LocationConfig.hpp"
# include <iostream>
# include <map>
# include <ostream>
# include <string>
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
	friend class			ConfigParser;

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
	vector<TListenConfig>	getListens() const;
	vector<string>			getServerNames() const;
	map<int, string>		getErrorPages() const;

	/*===Member Function===*/
	const LocationConfig	*Resolve(const std::string &uri)const;	///< La location matchant l'URI, prefixe le plus long
	string					build_path(const LocationConfig &location, const string &uri)const;	///< Traduit une URI en chemin disque via le root de la location

	/*===Friends===*/
	friend ostream			&operator<<(ostream &flux, const ServerConfig &src);
};

/*=== HELPERS ===*/
bool						is_segment_boundary(const string &uri, const string &path);
bool						is_all_digits(const string &s);
bool						is_valid_ipv4(const string &host);
bool						operator==(const TListenConfig &Listen_a, const TListenConfig &Listen_b);

#endif /*SERVER_CONFIG_HPP*/
