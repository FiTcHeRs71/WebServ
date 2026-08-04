#include "../../includes/ServerConfig.hpp"
#include <cstddef>
#include <ostream>
#include <string>

	/*===Canonical Form===*/
ServerConfig::ServerConfig(void)
{
	//std::cout << "ServerConfig default constructor called" << std::endl;
}

ServerConfig::~ServerConfig(void)
{
	//std::cout << "ServerConfig default destructor called" << std::endl;
}

ServerConfig::ServerConfig(const ServerConfig& to_copy)
	:_Listens(to_copy._Listens)
	,_ServerNames(to_copy._ServerNames)
	,_ErrorPages(to_copy ._ErrorPages)
	,_Locations(to_copy._Locations)
	,_ClientMaxBodySize(to_copy._ClientMaxBodySize)
{
	//std::cout << "ServerConfig copy constructor called" << std::endl;
}

ServerConfig	&ServerConfig::operator=(const ServerConfig& src)
{
	//std::cout << "ServerConfig operator assignement(=) constructor called" << std::endl;
	if (this != &src)
	{
		this->_Listens = src._Listens;
		this->_ServerNames = src._ServerNames;
		this->_ErrorPages = src._ErrorPages;
		this->_Locations = src._Locations;
		this->_ClientMaxBodySize = src._ClientMaxBodySize;
	}
	return (*this);
}

vector<pair<string, int> > ServerConfig::getListens() const{
	return (this->_Listens);
}

/**
 * @brief Surcharge d'operateur pour l'impression des attributs de la classe ServerConfig
 *
 * Affiche aussi chaque LocationConfig contenue dans le bloc.
 * @param flux Le flux de sortie a remplir.
 * @param src Le serveur dont on affiche les attributs.
 * @return le flux rempli
 */
ostream		&operator<<(ostream &flux, const ServerConfig &src)
{
	flux << "CONFIG SERVER :" << endl;
	for (size_t i = 0; i < src._Listens.size(); i++)
		flux << "Listens = " << src._Listens[i].first << ", " << src._Listens[i].second << endl;
	for (size_t i = 0; i < src._ServerNames.size(); i++)
		flux << "Server Name : " << src._ServerNames[i] << endl;
	for (map<int, string>::const_iterator it = src._ErrorPages.begin(); it != src._ErrorPages.end(); ++it)
		flux << "Error Pages : " << it->first << ", " << it->second << endl;
	for (size_t i = 0; i < src._Locations.size(); i++)
		flux << src._Locations[i] << endl;
	flux << "Client max body size = " << src._ClientMaxBodySize << endl;
	return (flux);
}

	/*===Member Function===*/
/**
 * @brief Trouve la configuration de location correspondant a une requete.
 *
 * @param host Le nom d'hote demande (issu de l'en-tete Host).
 * @param port Le port sur lequel la connexion a ete recue.
 * @param uri  L'URI demandee.
 * @return Un pointeur vers la LocationConfig correspondante, ou NULL si aucune ne correspond.
 */
const LocationConfig	*ServerConfig::Resolve(const std::string &uri)const
{
	const LocationConfig	*best = NULL;
	size_t					flag = uri.find_first_of("?#");
	string					uri_parsed;

	if (flag != string::npos)
		uri_parsed = uri.substr(0, flag);
	else
		uri_parsed = uri;

	for (size_t i = 0; i < this->_Locations.size(); i++)
	{
		const string &key = this->_Locations[i]._Path;

		if (uri_parsed.compare(0, key.size(), key) != 0)
			continue;
		if (!is_segment_boundary(uri_parsed, this->_Locations[i]._Path))
			continue;
		if(best == NULL || key.size() > best->_Path.size())
			best = &this->_Locations[i];
	}
	return (best);
}

/**
 * @brief Traduit une URI en chemin disque a partir de la location qui la matche.
 *
 * Retire le prefixe _Path de l'URI puis recolle le reste sur _Root, en
 * normalisant le '/' de jointure. Exemple du sujet :
 * "/kapouet/pouic/toto/pouet" avec location "/kapouet" et root "/tmp/www"
 * donne "/tmp/www/pouic/toto/pouet".
 * @param location La LocationConfig retournee par Resolve() pour cette URI.
 * @param uri L'URI demandee, query string et fragment tolerees.
 * @return Le chemin disque correspondant.
 */
string	ServerConfig::build_path(const LocationConfig &location, const string &uri)const
{
	size_t	flag = uri.find_first_of("?#");
	string	clean = (flag != string::npos) ? uri.substr(0, flag) : uri;
	string	root = location._Root;
	string	reste = clean.substr(location._Path.size());

	if (root.empty())
		return (reste);
	if (root[root.size() - 1] == '/' && !reste.empty() && reste[0] == '/')
		root = root.substr(0, root.size() - 1);
	else if (root[root.size() - 1] != '/' && !reste.empty() && reste[0] != '/')
		root += '/';

	return (root + reste);
}
