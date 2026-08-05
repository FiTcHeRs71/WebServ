#ifndef CONFIG_HPP
# define CONFIG_HPP

#include "LocationConfig.hpp"
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

const size_t		DEFAULT_BODY_SIZE	= 1048576;
const char *const	DEFAULT_ROOT		= "./www";
const char *const	DEFAULT_INDEX		= "index.html";
const char *const	DEFAULT_HOST		="0.0.0.0";
const int			DEFAULT_PORT		= 8080;

struct	TAddrPortGroup
{
	string			Host;
	int				Port;
	vector<size_t>	ServerIndexes;
	size_t			DefaultIndex;
};

/**
 * @brief Represente toute la phase de parsing du .conf
 *
 * Le parsing se fait en trois passes : tokenize() decoupe le fichier,
 * check_syntax() valide la structure, fill_servers_config() construit les objets.
 */
class ConfigParser
{
	private:

	vector<string>			_LexerConfig;	///< Le .conf sous forme de tokens non checkes
	vector<ServerConfig>	_Servers;		///< Un objet par bloc server valide
	vector<TAddrPortGroup>	_AddrPorts;

	public:

	/*===Canonical Form===*/
	ConfigParser(void);
	~ConfigParser(void);
	ConfigParser(const ConfigParser& to_copy);
	ConfigParser&operator=(const ConfigParser& src);

	/*===Getters & Setters===*/
	const vector<ServerConfig>	&getServers(void) const;				///< Les blocs server construits par fill_servers_config()

	/*===Member Function===*/
	void				check_syntax(const vector<string> &tokens);			///< Passe 2 : valide la structure des tokens
	void				check_listen(void);									///< Passe 4 : check les multi listen
	void				build_addr_port_groups(void);
	void				tokenize(const string &path);						///< Passe 1 : decoupe le .conf en tokens
	friend void			parse(const string &argv1, ConfigParser &Config);
	void				fill_servers_config(void);							///< Passe 3 : construit un ServerConfig par bloc
	void				fill_one_server(ServerConfig &server, size_t &i);	///< Remplit un seul bloc server
	void				apply_defaults(void);
};

/* === Helpers === */
vector<string>			collect_values(vector<string> &token, size_t &i);
void					is_valid_file(ifstream &file);
void					parse(const string &argv1, ConfigParser &Config);
const set<string>		&known_directives(void);
const set<string>		&known_methods(void);
pair<string, int>		parse_listen(const string &value); // TODO a virer
size_t					parse_body_size(const string &value);
map<int, string>		parse_error_pages(const vector<string> &value, size_t &j);
set<string>				parse_allow_methods(const vector<string> &value);
string					parse_root(const vector<string> &value);
vector<string>			parse_index(const vector<string> &value);
bool					parse_auto_index(const vector<string> &value);
string					parse_cgi_ext(const vector<string> &value);
string					parse_cgi_pass(const vector<string> &value);
int						parser_return_code(const string &value, const size_t nb_args);
string					parse_upload_store(const vector<string> &value);
vector<TListenConfig>	parse_listen_directive(const vector<string> &tokens);

#endif /*CONFIG_HPP*/