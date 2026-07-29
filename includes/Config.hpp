#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "ServerConfig.hpp"
# include <cstddef>
# include <iostream>
# include <string>
# include <vector>
# include <set>
# include <fstream>
# include <utility>
# include <map>

using namespace std;

/**
 * @brief Represente toute la phase de parsing du .conf
 *
 * _LexerConfig contient le .conf sous forme de tokens non checkes
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
	
};

/* === Helpers === */
vector<string>		collect_values(vector<string> &token, size_t &i);
void				is_valid_file(ifstream &file);
void				parse(const string &argv1);
const set<string>	&known_directives(void);
const set<string>	&known_methods(void);
pair<string, int>	parse_listen(const string &value);
size_t				parse_body_size(const string &value);
map<int, string>	parse_error_pages(const vector<string> &value, size_t &j);
set<string>			parse_allow_methods(const vector<string> &value);
string				parse_root(const vector<string> &value);
vector<string>		parse_index(const vector<string> &value);
bool				parse_auto_index(const vector<string> &value);
string				parse_cgi_ext(const vector<string> &value);
string				parse_cgi_pass(const vector<string> &value);
int					parser_return_code(const string &value, const size_t nb_args);

#endif /*CONFIG_HPP*/