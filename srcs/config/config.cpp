#include "../../includes/Config.hpp"
#include "../../includes/ServerConfig.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

ConfigParser::ConfigParser(void)
{
	//cout << "ConfigParser default constructor called" << endl;
}
ConfigParser::~ConfigParser(void)
{
	//cout << "ConfigParser default destructor called" << endl;
}
ConfigParser::ConfigParser(const ConfigParser& to_copy)
	:_LexerConfig(to_copy._LexerConfig)
	,_Servers(to_copy._Servers)
	,_AddrPorts(to_copy._AddrPorts)
{
	//cout << "ConfigParser copy constructor called" << endl;
}
ConfigParser & ConfigParser::operator=(const ConfigParser& src)
{
	//cout << "ConfigParser operator assignement(=) constructor called" << endl;
	if (this != &src)
	{
		this->_LexerConfig = src._LexerConfig;
		this->_Servers = src._Servers;
		this->_AddrPorts = src._AddrPorts;
	}
	return (*this);
}

/**
 * @brief Accesseur sur les blocs server construits par le parsing.
 *
 * Valide uniquement apres un appel a fill_servers_config().
 * @return La liste des ServerConfig, vide si le parsing n'a pas eu lieu.
*/
const vector<ServerConfig>	&ConfigParser::getServers(void) const
{
	return (this->_Servers);
}

/**
 * @brief Fonction d'entree pour le parsing et le checking
 *
 * Enchaine les trois passes : tokenize -> check_syntax -> fill_servers_config
 * Le ConfigParser est fourni par l'appelant pour que les ServerConfig
 * lui survivent (voir getServers()).
 * @param argv1 Le chemin du fichier .conf passe au programme.
 * @param Config Le parser a remplir.
 * @return void, throw a la premiere erreur rencontree
*/
void	parse(const string &argv1, ConfigParser &Config)
{
	Config.tokenize(argv1);
	Config.check_syntax(Config._LexerConfig);
	Config.fill_servers_config();
	Config.check_listen();
	Config.apply_defaults();
}

/**
 * @brief Prend le fichier de configuration et le tokenize.
 * 
 * Il stocke les valeurs dans _LexerConfig, les commentaires (#) sont retires et
 * les caracteres speciaux (";{}") sont isoles comme des tokens a part entiere.
 * Il ne fait aucune verification de validite ou de syntaxe des arguments
 * Checking au debut de l'accessibilite du fichier passe en argument
 * @param path Le chemin du fichier .conf a lire.
 * @return void
*/
void	ConfigParser::tokenize(const string &path)
{
	ifstream		config_file(path.c_str());
	string			line;
	string			to_ignore = " \t\n\r\v\f";
	string			specials = ";{}";

	is_valid_file(config_file);

	while (getline(config_file, line))
	{
		size_t	flag_begin = 0;
		size_t	flag_end = 0;
		size_t	comment = line.find("#");
		
		if (comment != string::npos)
			line.erase(comment);
		size_t	first = line.find_first_not_of(to_ignore);
		if (first == string::npos)
			continue;
		while (flag_end < line.size())
		{
			flag_begin = line.find_first_not_of(to_ignore, flag_begin);
			if (flag_begin == string::npos)
				break;
			flag_end = (line.find_first_of(to_ignore + specials, flag_begin));
			if (flag_end != string::npos && specials.find(line[flag_end]) != string::npos)
			{
				if (flag_end > flag_begin)
					this->_LexerConfig.push_back(line.substr(flag_begin, flag_end - flag_begin));
				this->_LexerConfig.push_back(line.substr(flag_end, 1));
				flag_begin = flag_end + 1;
			}
			else if (flag_end - flag_begin > 0)
			{
				this->_LexerConfig.push_back(line.substr(flag_begin, flag_end - flag_begin));
				flag_begin = flag_end;
			}
		}
	}
}

/**
 * @brief Prend la liste de tokens et verifie la syntaxe
 * 
 * Il parcourt la liste de tokens generee par la fonction tokenize et verifie l'ordre des arguments
 * Controle l'equilibre des accolades, l'imbrication des blocs, la presence
 * d'une valeur et du ";" apres chaque directive
 * Il ne fait aucune verification sur la validite des ports ou autres
 * @param tokens La liste de tokens produite par tokenize().
 * @return void, throw en cas de syntaxe invalide
*/
void	ConfigParser::check_syntax(const vector<string> &tokens)
{
	vector<string>	block_stack;
	size_t			i = 0;

	while (i < tokens.size())
	{
		const string &tok = tokens[i];

		if (tok == "}")
		{
			if (block_stack.empty())
				throw invalid_argument("Closing brace expected");
			block_stack.pop_back();
			i++;
			continue;
		}
		else if (!known_directives().count(tok))
			throw runtime_error("directive inconnue : " + tok);
		if (tok == "server" && !block_stack.empty())
			throw runtime_error("Cannot have a server block inside a otherone");
		if (tok == "location" && (block_stack.empty() || block_stack.back() != "server"))
			throw runtime_error("Location block must be inside a server block");
		if (tok != "server" && tok != "location" && block_stack.empty())
			throw runtime_error(tok + "is out side on any blocks");

		i++;

		if (tok == "server" || tok == "location")
		{
			if (tok == "location")
			{
				if (i >= tokens.size() || tokens[i] == "{")
					throw runtime_error("Location without PATH");
				i++;
			}
			if (i >= tokens.size() || tokens[i] != "{")
				throw runtime_error("'{' expected after " + tok);
			block_stack.push_back(tok);
			i++;
			continue;
		}

		size_t	value_count = 0;

		while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "}"
			&& tokens[i] != "{" && !known_directives().count(tokens[i]))
		{
			value_count++;
			i++;
		}
		if (value_count == 0)
			throw runtime_error("directive '" + tok + "' without value");
		if (i >= tokens.size() || tokens[i] != ";")
			throw runtime_error("';' missing after '" + tok + "'");
		i++;
	}
	if (!block_stack.empty())
		throw runtime_error("Brace not closed");
}

/**
 * @brief Fonction d'entree pour le parsing de chaque bloc serveur
 *
 * Il parcourt la liste de tokens generee par la fonction tokenize
 * Il remplit le vector _Servers avec des objets ServerConfig.
 * ServerConfig contient tous les elements de chaque bloc serveur
 * @return void, throw si un token traine hors d'un bloc server
*/
void	ConfigParser::fill_servers_config(void)
{
	size_t i = 0;

	while (i < this->_LexerConfig.size())
	{
		if (this->_LexerConfig[i] != "server")
			throw runtime_error("'" + this->_LexerConfig[i] + "' outside of a server block");
		i += 2; // skip token "server" + "{"
		ServerConfig	server;
		fill_one_server(server, i);
		if (i >= this->_LexerConfig.size())
			throw runtime_error("server block not closed");
		i++;
		this->_Servers.push_back(server);
	}
}

/**
 * @brief Prend un objet server instancie dans fill_servers_config et remplit ses attributs
 *
 * Il parcourt la liste de tokens genere par la fonction 'tokenize'
 * Puis trouve les directives et remplit les attributs avec les valeurs associees
 * Refuse les directives dupliquees (sauf location et error_page) et les
 * couples ip:port deja utilises dans ce bloc ou dans un bloc server precedent
 * Delegue les blocs location a LocationConfig::parse_location()
 * @param server L'objet a remplir, instancie par fill_servers_config().
 * @param i Index positionne apres le "{" du bloc, avance jusqu'au "}" final.
 * @return void, throw sur toute valeur ou directive invalide
*/
void	ConfigParser::fill_one_server(ServerConfig &server, size_t &i)
{
	set<string>	seen;

	while (i < this->_LexerConfig.size() && this->_LexerConfig[i] != "}")
	{
		string	key = this->_LexerConfig[i];
		if (!seen.insert(key).second && key != "location" && key != "error_page" && key != "listen")
			throw runtime_error(key + " multiple definition not allowed");
		i++;

		if (key == "location")
		{
			LocationConfig	location;
			location.parse_location(this->_LexerConfig, i);
			server._Locations.push_back(location);
		}
		else
		{
			vector<string>	value = collect_values(this->_LexerConfig, i);
			if (key == "listen")
			{
				vector<TListenConfig>	listens = parse_listen_directive(value);
				for (size_t k = 0; k < listens.size(); k++)
					server._Listens.push_back(listens[k]);
				continue;
			}
			for (size_t j = 0; j < value.size(); j++)
			{
				if (key ==  "server_name")
					server._ServerNames.push_back(value[j]); 
				else if (key == "client_max_body_size")
				{
					if (server._HasClientMaxBodySize)
						throw runtime_error("Multiple definition of <client_max_body_size> key in server block");
					server._ClientMaxBodySize = parse_body_size(value[j]);
					server._HasClientMaxBodySize = true;
				}
				else if (key == "error_page")
				{
					map<int, string>	tmp = parse_error_pages(value, j);
					server._ErrorPages.insert(tmp.begin(), tmp.end());
				}
				else
					throw runtime_error("directive : '" + key + "' forbiden in server body");
			}
		}
	}
}

/**
 * @brief Parcours la liste des serveurs du .conf et initilalize les valeurs critiques
 *
 * Il parcourt la liste de serveurs et regarde les valeurs de ou la declaration de <location / {...}>,
 * Pour le client_max_body_size si non itinialiser appplique la valeur par default de NGINX
 * Pour root il met par defaut le chemin vers www
 * Pour methods il attribue automatchiquement GET
 * Pour index il attribue automatchiquement "index.html"
 * Toutes valeur sont dans le header config.hpp
 * @param void void.
 * @return void.
*/
void	ConfigParser::apply_defaults(void)
{
	for (size_t i = 0; i < this->_Servers.size(); i++)
	{
		ServerConfig	&srv= this->_Servers[i];
		bool			location_flag = false;

		if (!srv._HasClientMaxBodySize)
			srv._ClientMaxBodySize = DEFAULT_BODY_SIZE;
		for (size_t k = 0; k < srv._Locations.size(); k++)
		{
			if (srv._Locations[k]._Path == "/")
			{
				location_flag = true;
				break;
			}
		}
		if (!location_flag)
		{
			LocationConfig	default_location;
			default_location._Path = "/";
			srv._Locations.push_back(default_location);
		}
		if (srv._Listens.size() < 1)
		{
			TListenConfig	default_listens;
			default_listens.Host = DEFAULT_HOST;
			default_listens.Port = DEFAULT_PORT;
			srv._Listens.push_back(default_listens);
		}
		cout << srv << endl;
		for (size_t j = 0; j < srv._Locations.size(); j++)
		{
			if (srv._Locations[j]._Index.empty())
				srv._Locations[j]._Index.push_back(DEFAULT_INDEX);
			if (srv._Locations[j]._Root.empty())
				srv._Locations[j]._Root = DEFAULT_ROOT;
			if (srv._Locations[j]._Methods.empty())
				srv._Locations[j]._Methods.insert("GET");
			if (!srv._Locations[j]._HasClientMaxBodySize)
				srv._Locations[j]._ClientMaxBodySize = srv._ClientMaxBodySize;
		}
	}
}

/**
 * @brief Parcours tous les lsitens declarer dans .conf et check les doublon
 *
 * Parcours la liste des listens declarer et verifie que les serveurs ayant le meme listen ont un 
 * <server_name> different
 * @param void void.
 * @return void + throw une exception en cas de listen invalid.
*/
void	ConfigParser::check_listen(void)
{
	for (size_t a = 0; a < this->_Servers.size(); a++) // conflit = meme host:port ET un server_name en commun (deux blocs sans server_name = conflit aussi : meme default server)
		for (size_t b = a + 1; b < _Servers.size(); b++)
			for (size_t la = 0; la < _Servers[a]._Listens.size(); la++)
			{
				if (find(_Servers[b]._Listens.begin(), _Servers[b]._Listens.end(), _Servers[a]._Listens[la]) == _Servers[b]._Listens.end())
					continue;
				if (_Servers[a]._ServerNames.empty() && _Servers[b]._ServerNames.empty())
					throw runtime_error("conflicting default server on same host:port");
				for (size_t n = 0; n < _Servers[a]._ServerNames.size(); n++)
					if (find(_Servers[b]._ServerNames.begin(), _Servers[b]._ServerNames.end(), _Servers[a]._ServerNames[n]) != _Servers[b]._ServerNames.end())
						throw runtime_error("conflicting server name '" + _Servers[a]._ServerNames[n] + "'");
			}
}

void	ConfigParser::build_addr_port_groups(void)
{
	this->_AddrPorts.clear();

	for (size_t i = 0; i < this->_Servers.size(); i++)
	{
		const vector<TListenConfig>	&listens = this->_Servers[i]._Listens;

		for (size_t l = 0; l < listens.size(); l++)
		{
			size_t goal;

			if (find(listens.begin(), listens.begin() + l, listens[l]) != listens.begin() + l)
			{
				ostringstream	oss;

				oss << "duplicated listen " << listens[l].Host << ":" << listens[l].Port;
				throw runtime_error(oss.str());
			}
			goal = this->_AddrPorts.size();
			for (size_t a = 0; a < this->_AddrPorts.size(); a++)
			{
				if (this->_AddrPorts[a].Port == listens[l].Port)
				{
					goal = a;
					break;
				}
			}
			if (goal == this->_AddrPorts.size())
			{
				TAddrPortGroup group;

				group.Host = listens[l].Host;
				group.Port = listens[l].Port;
				group.DefaultIndex = this->_Servers.size();
				this->_AddrPorts.push_back(group);
			}
		}
	}
}