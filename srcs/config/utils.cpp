#include "../../includes/Config.hpp"
#include <fstream>
#include <iostream>
#include <set>

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