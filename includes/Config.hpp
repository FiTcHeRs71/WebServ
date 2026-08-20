#ifndef CONFIG_HPP
# define CONFIG_HPP

# include "Struct_webserv.hpp"
# include "Default.hpp"
# include "LocationConfig.hpp"
# include "ServerConfig.hpp"
# include <cstddef>
# include <iostream>
# include <string>
# include <vector>
# include <set>
# include <fstream>
# include <map>

using namespace std;

/**
 * @brief Represente toute la phase de parsing du .conf
 *
 * Le parsing se fait en cinq passes, enchainees par parse() :
 *   1. tokenize()              decoupe le fichier en tokens
 *   2. check_syntax()          valide la structure (accolades, ";", imbrication)
 *   3. fill_servers_config()   construit un ServerConfig par bloc server
 *   4. apply_defaults()        comble les directives absentes
 *   5. build_addr_port_groups() regroupe les servers par socket a ouvrir
 *
 * L'ordre compte : les passes 4 et 5 sont dans cet ordre pour que les listen
 * implicites soient soumis aux memes controles de conflit que les explicites.
 */
class ConfigParser
{
	private:

	vector<string>			_LexerConfig;	///< Le .conf sous forme de tokens non checkes
	vector<ServerConfig>	_Servers;		///< Un objet par bloc server valide
	vector<TAddrPortGroup>	_AddrPorts;		///< Les struct contenant les serveur ecoutant les memes ports

	public:

	/*===Canonical Form===*/
	ConfigParser(void);
	~ConfigParser(void);
	ConfigParser(const ConfigParser& to_copy);
	ConfigParser&operator=(const ConfigParser& src);

	/*===Getters & Setters===*/
	const vector<ServerConfig>		&getServers(void) const;					///< Les blocs server construits par fill_servers_config()
	const vector<TAddrPortGroup>	&getAddrPorts(void) const;					///< La table des sockets d'ecoute

	/*===Member Function===*/
	void				check_syntax(const vector<string> &tokens);			///< Passe 2 : valide la structure des tokens
	void				build_addr_port_groups(void);						///< Passe 5 : regroupe les servers par host:port et detecte les conflits
	void				tokenize(const string &path);						///< Passe 1 : decoupe le .conf en tokens
	friend void			parse(const string &argv1, ConfigParser &Config);
	void				fill_servers_config(void);							///< Passe 3 : construit un ServerConfig par bloc
	void				fill_one_server(ServerConfig &server, size_t &i);	///< Remplit un seul bloc server
	void				apply_defaults(void);								///< Passe 4 : comble les directives absentes avec Default.hpp
	void				resolve_paths(void);								///< Passe 5 : rend absolus tous les chemins disque de la conf
};

/* === Helpers === */
vector<string>			collect_values(vector<string> &token, size_t &i);
void					is_valid_file(ifstream &file);
void					check_valid_path(const string &path);
void					parse(const string &argv1, ConfigParser &Config);
const set<string>		&known_directives(void);
const set<string>		&known_methods(void);
const set<string>		&known_listen_parameters(void);
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
string					to_absolute(const string &path, const string &cwd);
const ServerConfig		&SelectServer(const ConfigParser &config, size_t group_index, const string &host_header);

#endif /*CONFIG_HPP*/