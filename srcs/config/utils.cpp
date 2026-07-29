#include "../../includes/Config.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using namespace std;

/**
 * @brief Check si le fichier passe en argument du programme est ouvrable / lisible et non vide.
 * @return Return l'argument de la ligne en cours de checking.
*/
void	is_valid_file(ifstream &file)
{
	if (!file.is_open())
		throw invalid_argument("Cannot open configuration file");
	if (file.peek() == ifstream::traits_type::eof())
		throw invalid_argument("configuration file is empty");
}

/**
 * @brief Contient tous les arguments (key) valides passables au programme
 * ex : ->listen<-	0.0.0.0:8080;
 * @return Return l'argument de la ligne en cours de checking.
*/
const set<string> &known_directives(void)
{
	static const string names[] = {
		"listen", "server_name", "client_max_body_size", "error_page",
		"allow_methods", "root", "index", "autoindex",
		"cgi_ext", "cgi_pass", "return", "location", "server",
	};
	static const set<string> s(names, names + sizeof(names) / sizeof(names[0]));
	return (s);
}

/**
 * @brief Split le token value associe a la key listen dans une pair.
 * La pair contient le host puis le port (0.0.0.0 | 8080)
 * Check si la valeur depasse INT_MAX ou si elle est negative.
 * @return la pair completee
*/
pair<string, int>	parse_listen(const string &value)
{
	pair<string, int>	pair;
	size_t				flag;
	string				host;
	string				port;

	flag = value.find_first_of( ":");
	host = value.substr(0, flag);
	port = value.substr(flag + 1);

	char*				p_end = NULL;
	errno = 0;
	long				port_converted = strtol(port.c_str(), &p_end, 10);

	if (port_converted > INT_MAX || port_converted < 0 || errno == ERANGE)
		throw runtime_error( port + " is not a valid port");
	pair.first = host;
	pair.second = static_cast<int>(port_converted);
	return (pair);
}

/**
 * @brief Fait la conversion de la value donnee par client_max_body_size en size_t
 * Check si une lettre de size est presente en fin de value (KkMmGg).
 * Check si la value n'est pas empty ou trop grande.
 *
 * @return le size_t converti
*/
size_t	parse_body_size(const string &value)
{
	int		multiplier;
	long	size_converted;
	char*	p_end = NULL;
	errno = 0;

	if (value.empty())
		throw runtime_error(value + " is not a valid body size");
	size_converted = strtol(value.c_str(), &p_end, 10);
	if (p_end == value.c_str() || size_converted < 0 || errno == ERANGE)
		throw runtime_error(value + " is not a valid body size");
	if (*p_end == '\0')
		multiplier = 1;
	else if ((*p_end == 'K' || *p_end == 'k') && p_end[1] == '\0')
		multiplier = 1024;
	else if ((*p_end == 'M' || *p_end == 'm') && p_end[1] == '\0')
		multiplier = 1024 * 1024;
	else
		throw runtime_error(value + " is not a valid body size");
	if (size_converted > INT_MAX / multiplier)
		throw runtime_error(value + " is a too big body size");
	return (static_cast<size_t>(size_converted));
}

/**
 * @brief Cree une map STL contenant le code d'erreur associe au PATH de la page dediee
 * Check si le code d'erreur depasse 505 ou si la value est negative ou contient des caracteres
 * non numeriques.
 *
 * @return une map[<Error_code>] = "PATH"
*/
map<int, string>	parse_error_pages(const vector<string> &value, size_t &j)
{
	map<int, string>	map;
	long	size_converted;
	char*	p_end = NULL;
	
	if (value.size() < 2)
		throw runtime_error("Error pages missing a elements, minimum correct value needed is 2 or more");
	while (j < value.size() - 1)
	{
		errno = 0;
		size_converted = strtol(value[j].c_str(), &p_end, 10);
		if (size_converted > 505 || size_converted < 0 || errno == ERANGE || *p_end != '\0')
			throw runtime_error( value[j] + " is not a valid code page error");
		map[static_cast<int>(size_converted)] = value.back();
		j++;
	}
	return (map);
}

/**
 * @brief Remplit un vecteur de toutes les valeurs associees a une key
 *
 * Remplit un vecteur de tous les elements contenus entre la key et ";"
 * @return vecteur rempli des valeurs
*/
vector<string>		collect_values(vector<string> &token, size_t &i)
{
	vector<string>	values;

	while (token[i] != ";")
	{
		values.push_back(token[i]);
		i++;
	}
	i++; // saute le ";"
	return (values);
}

const set<string> &known_methods(void)
{
	static const string names[] = {
		"GET", "POST", "DELETE"
	};
	static const set<string> s(names, names + sizeof(names) / sizeof(names[0]));
	return (s);
}

set<string>	parse_allow_methods(const vector<string> &value)
{
	set<string>	set;

	for (size_t i = 0; i < value.size(); i++)
	{
		if (!known_methods().count(value[i]))
			throw runtime_error("Unknow directives : " + value[i]);
		set.insert(value[i]);
	}
	return (set);
}

string	parse_root(const vector<string> &value)
{
	if (value.size() > 1)
		throw runtime_error("Too much arguments for key <ROOT>");
	return (value[0]);
}

vector<string>	parse_index(const vector<string> &value)
{
	if (value.size() == 0)
		throw runtime_error("Key index in location bloc need at leat one value");
	for (size_t i = 0; i < value.size(); i++)
	{
		if (value[i].empty())
			throw runtime_error ("Key index in location bloc has a empty arguments");
		//if (value.)// doubon ?
	}
	return (value);
}

bool	parse_auto_index(const vector<string> &value)
{
	if (value.size() != 1)
		throw runtime_error("autoindex needs only one arguments");
	if (value[0] != "on" && value[0] != "off")
		throw runtime_error(value[0] + "is not a valid argument for key autoindex");
	if (value[0] == "on")
		return (true);
	return (false);
}

string	parse_cgi_ext(const vector<string> &value)
{
	if (value.size() != 1)
		throw runtime_error("cgi_ext takes exactly one extension");
	if (value[0].size() < 2 || value[0][0] != '.')
		throw runtime_error(value[0] + " is not a valid CGI extensions");
	return (value[0]);
}

string	parse_cgi_pass(const vector<string> &value)
{
	if (value.size() != 1)
		throw runtime_error("cgi_pass takes exactly one extension");
	//check si le path du cgi est bon ?
	return (value[0]);
}

int	parser_return_code(const string &value, const size_t nb_args)
{
	for (size_t i = 0; i < value.size(); i++)
	{
		if(!isdigit(static_cast<unsigned char>(value[i])))
			throw runtime_error(value + " is not a valid return code");
	}
	char	*end;
	errno = 0;
	long	code = strtol(value.c_str(), &end, 10);
	if (code > 299 && code < 400 && nb_args == 1)
		throw runtime_error (value + " needs to have target <URL>");
	if (code < 100 || code > 599 || errno == ERANGE || *end != '\0')
		throw runtime_error(value + "is not a valid HTTP status code");
	return (static_cast<int>(code));
}

string	parse_upload_store(const vector<string> &value)
{
	if (value.size() != 1)
		throw runtime_error("upload_store in location block needs only one value");
	return (value[0]);
}