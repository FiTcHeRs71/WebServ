#include "../../includes/ServerConfig.hpp"
#include <ostream>

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
	,_ErrorPages(to_copy._ErrorPages)
	,_Locations(to_copy._Locations)
	,_ClientMaxBodySize(to_copy._ClientMaxBodySize)
{
	//std::cout << "ServerConfig copy constructor called" << std::endl;
}

ServerConfig	&ServerConfig::operator=(const ServerConfig& src)
{
	//std::cout << "ServerConfig operator assignement(=) constructor called" << std::endl;
	(void)src;
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

	/*===Member Function===*/
/**
 * @brief Trouve la configuration de location correspondant a une requete.
 * @param host Le nom d'hote demande (issu de l'en-tete Host).
 * @param port Le port sur lequel la connexion a ete recue.
 * @param uri  L'URI demandee.
 * @return Un pointeur vers la LocationConfig correspondante, ou NULL si aucune ne correspond.
 */
const LocationConfig	*ServerConfig::Resolve(const std::string &host, int port, const std::string &uri)
{
	(void)host;
	(void)port;
	(void)uri;
	return (NULL);
}

/**
 * @brief Surcharge d'operateur pour l'impression des attributs de la classe ServerConfig
 *
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
