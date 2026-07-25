#ifndef SERVER_CONFIG_HPP
# define SERVER_CONFIG_HPP

# include "LocationConfig.hpp"
# include "Config.hpp"
# include <iostream>

/**
 * @brief Represente la configuration d'un serveur virtuel (bloc "server").
 *
 * Regroupe les hosts/ports ecoutes ainsi que l'ensemble des
 * LocationConfig associees, et permet de resoudre la location
 * correspondant a une requete donnee.
 */
class ServerConfig
{
	friend class ConfigParser;
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
	const LocationConfig	*Resolve(const std::string &host, int port, const std::string &uri);
};

#endif /*SERVER_CONFIG_HPP*/