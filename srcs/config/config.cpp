#include "../../includes/Config.hpp"
#include "../../includes/ServerConfig.hpp"
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <stdexcept>
#include <string>
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
{
	//cout << "ConfigParser copy constructor called" << endl;
}
ConfigParser & ConfigParser::operator=(const ConfigParser& src)
{
	//cout << "ConfigParser operator assignement(=) constructor called" << endl;
	if (this != &src)
	{
		this->_LexerConfig = src._LexerConfig;
	}
	return (*this);
}

/**
 * @brief Fonction d'entrer pour le parsing et les checking
 * 
 * Remplir avec les differents appel au fonction de checking
 * @return void
*/
void	parse(const string &argv1)
{
	ConfigParser	Config;

	Config.tokenize(argv1);
	Config.check_syntax(Config._LexerConfig);
	Config.fill_servers_config();
}

/**
 * @brief Prend le fichier de configuration et le tokenize.
 * 
 * Il stocke les valeurs dans dans un vector (lexer_config)
 * Il ne fait aucune verification de validite ou synthaxes des arguments
 * Checking au debut de l'accesiblite du file passer en argument
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
 * Il parcours la liste de tokens generer par la fonction tokenize et verifie l'ordre des arguments
 * Il ne fait aucune verificatiion sur la validite des ports ou autres
 * throw une exception en cas de syntaxe invalid
 * @return void
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

void	ConfigParser::fill_servers_config(void)
{
	size_t i = 0;

	while (i < this->_LexerConfig.size())
	{
		i += 2; // skip token "server" + "{"
		ServerConfig	server;
		fill_one_server(server, i);
		this->_Servers.push_back(server);
	}
}

void	ConfigParser::fill_one_server(ServerConfig &server, size_t &i)
{
	while (this->_LexerConfig[i] != "}")
	{
		string	key = this->_LexerConfig[i];
		i++;

		if (key == "location") //ticket A-03
			continue;
		else
		{
			vector<string>	value = collect_values(i);
			for (int j = 0; i < value.size(); i++)
			{
				if (key == "listen")
					server._Listens.push_back(parse_listen(value[j++])); // split 0.0.0.0 de 8080
				else if (key ==  "server_name")
					server._ServerNames.push_back(value[j++]);
				/*else if (key == "client_max_body_size")
					server._ClientMaxBodySize = (strtol(value[++i])); // ajouter check si value corret ex pas de lettre
				else if (key == "error_page")
					server._ErrorPages.insert(static_cast<int>(strtol(value[++i])), value[++i]); //ajouter check si value corret ex pas de lettre ou range
				else
					throw runtime_error("directive '" + key + "' interdite dans server");*/
			}
		}
		i++;
	}
}

vector<string>	ConfigParser::collect_values(size_t &i)
{
	vector<string>	values;

	while (_LexerConfig[i] != ";")
	{
		values.push_back(_LexerConfig[i]);
		i++;
	}
	i++;
	return (values);
}