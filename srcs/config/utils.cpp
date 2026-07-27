#include "../../includes/Config.hpp"
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
 * @brief check si le fichier passer en argument du programme est ouvrable / lisible et non-vide.
*/
void	is_valid_file(ifstream &file)
{
	if (!file.is_open())
		throw invalid_argument("Cannot open configuration file");
	if (file.peek() == ifstream::traits_type::eof())
		throw invalid_argument("configuration file is empty");
}

/**
 * @brief contient toutes les arguments(key) valide passable aux programme
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
 * @return Return la pair completer
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
 * @brief fait la conversion entre la value donner par client_max_body_size en size_t
 * Check si une lettre de size est presente en fin value (KkMmGg)
 * 
 * @return Return le size_t convertie
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

map<int, string>	parse_error_pages(const vector<string> value, size_t &j)
{
	map<int, string>	map;
	long	size_converted;
	char*	p_end = NULL;
	
	if (value.size() < 2)
		throw ("Error pages missing a elements, minimum correct value needed is 2 or more");
	while (j < value.size() - 1)
	{
		errno = 0;
		size_converted = strtol(value[j].c_str(), &p_end, 10);
		if (size_converted > 600 || size_converted < 0 || errno == ERANGE || *p_end != '\0')
			throw runtime_error( value[j] + " is not a valid code page error");
		map[static_cast<int>(size_converted)] = value.back();
		j++;
	}
	return (map);
}