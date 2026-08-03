#ifndef LOCATION_CONFIG_HPP
# define LOCATION_CONFIG_HPP

# include <iostream>
# include <map>
# include <string>
# include <vector>
# include <set>

using namespace std;

/**
 * @brief Represente la configuration d'une location (bloc "location") d'un serveur.
 *
 * Definit les regles applicables a un chemin donne : racine, index,
 * methodes autorisees, redirection, configuration CGI, etc.
 */
class LocationConfig
{
	friend class		ServerConfig;
	private:

	string				_Path;
	set<string>			_Methods;
	string				_Root;
	vector<string>		_Index;
	bool				_AutoIndex;
	int					_ReturnCode;
	bool				_HasReturn;
	string				_CgiExt;
	string				_CgiPass;
	size_t				_ClientMaxBodySize;
	bool				_HasClientMaxBodySize;
	string				_ReturnTarget;
	string				_UploadStore;
	bool				_HasUploadStore;

	protected:



	public:

	/*===Canonical Form===*/
	LocationConfig(void);
	~LocationConfig(void);
	LocationConfig(const LocationConfig& to_copy);
	LocationConfig &operator=(const LocationConfig& src);

	/*===Getters & Setters===*/
	const string	&getPath(void) const;	///< Le prefixe d'URI declare par le bloc "location"
	const string	&getRoot(void) const;	///< La racine disque associee a ce bloc

	/*===Member Function===*/
	void	parse_location(vector<string>	&token, size_t &i);

	friend ostream	&operator<<(ostream &flux, const LocationConfig &src);
};

#endif /*LOCATION_CONFIG_HPP*/