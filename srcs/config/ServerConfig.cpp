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
const LocationConfig	*ServerConfig::resolve(const std::string &host, int port, const std::string &uri)
{
	(void)host;
	(void)port;
	(void)uri;
	return (NULL);
}