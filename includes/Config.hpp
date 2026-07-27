#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "ServerConfig.hpp"
#include <cstddef>
# include <iostream>
#include <string>
# include <vector>
# include <set>

using namespace std;

/**
 * @brief Represente toute la pahse de parsing du .conf
 *
 * _LexerConfig contient sous forme de token non checker le .conf
 */
class ConfigParser
{
	private:

	vector<string>			_LexerConfig;
	vector<ServerConfig>	_Servers;

	public:

	/*===Canonical Form===*/
	ConfigParser(void);
	~ConfigParser(void);
	ConfigParser(const ConfigParser& to_copy);
	ConfigParser&operator=(const ConfigParser& src);

	/*===Getters & Setters===*/
	

	/*===Member Function===*/
	void			check_syntax(const vector<string> &tokens);
	void			tokenize(const string &path);
	friend void		parse(const string &argv1);
	void			fill_servers_config(void);
	void			fill_one_server(ServerConfig &server, size_t &i);
	vector<string>	collect_values(size_t &i);

};

/* === Helpers === */
void				is_valid_file(ifstream &file);
void				parse(const string &argv1);
const set<string>	&known_directives();
pair<string, int>	parse_listen(string	value);

#endif /*CONFIG_HPP*/