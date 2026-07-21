#include "../../includes/ServerConfig.hpp"

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
{
	//std::cout << "ServerConfig copy constructor called" << std::endl;
	(void)to_copy;
}

ServerConfig	&ServerConfig::operator=(const ServerConfig& src)
{
	//std::cout << "ServerConfig operator assignement(=) constructor called" << std::endl;
	(void)src;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}

	/*===Getters & Setters===*/


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