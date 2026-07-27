#include "../../includes/Config.hpp"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>
#include <stdexcept>
#include <utility>

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

pair<string, int>	parse_listen(string	value)
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
