#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include "LocationConfig.hpp"
# include <iostream>
# include <string>
# include <vector>

class ServerConfig
{
	private:



	protected:



	public:

	/*===Canonical Form===*/
	ServerConfig(void);
	~ServerConfig(void);
	ServerConfig(const ServerConfig& to_copy);
	ServerConfig &operator=(const ServerConfig& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/
	const LocationConfig	*resolve(const std::string &host, int port, const std::string &uri);
};

#endif /*SERVER_CONFIG_HPP*/