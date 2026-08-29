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
	friend class		ConfigParser;

	private:

	string			_Path;
	set<string>		_Methods;
	string			_Root;
	vector<string>	_Index;
	bool			_AutoIndex;
	int				_ReturnCode;
	bool			_HasReturn;
	string			_CgiExt;
	string			_CgiPass;
	size_t			_ClientMaxBodySize;
	bool			_HasClientMaxBodySize;
	string			_ReturnTarget;
	string			_UploadStore;
	bool			_HasUploadStore;

	public:

	/*===Canonical Form===*/
	LocationConfig(void);
	~LocationConfig(void);
	LocationConfig(const LocationConfig& to_copy);
	LocationConfig &operator=(const LocationConfig& src);

	/*===Getters & Setters===*/
	const string					&getPath(void) const;	///< Le prefixe d'URI declare par le bloc "location"
	const string					&getRoot(void) const;	///< La racine disque associee a ce bloc
	const string					&getExt(void) const;
	const string					&getPass(void) const;
	const size_t					&getClientMaxBodySize(void) const;
	const std::set<std::string>		&getMethods(void) const;
	const std::vector<std::string>	&getIndex(void) const;
	bool							getAutoIndex(void) const;
	bool							hasReturn(void) const;
	int								getReturnCode(void) const;
	const std::string				&getReturnTarget(void) const;
	const std::string				&getUploadStore(void) const;
	bool							hasUploadStore(void) const;

	/*===Member Function===*/
	void							parse_location(vector<string>	&token, size_t &i);

	friend ostream					&operator<<(ostream &flux, const LocationConfig &src);
};

#endif /*LOCATION_CONFIG_HPP*/