#ifndef LOCATION_CONFIG_HPP
# define LOCATION_CONFIG_HPP

# include <iostream>

/**
 * @brief Represente la configuration d'une location (bloc "location") d'un serveur.
 *
 * Definit les regles applicables a un chemin donne : racine, index,
 * methodes autorisees, redirection, configuration CGI, etc.
 */
class LocationConfig
{
	private:



	protected:



	public:

	/*===Canonical Form===*/
	LocationConfig(void);
	~LocationConfig(void);
	LocationConfig(const LocationConfig& to_copy);
	LocationConfig &operator=(const LocationConfig& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/

};

#endif /*LOCATION_CONFIG_HPP*/