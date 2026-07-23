#include "../../includes/config.hpp"
#include <fstream>
#include <iostream>
#include <set>

using namespace std;

void	is_valid_file(ifstream &file)
{
	if (!file.is_open())
		throw invalid_argument("Cannot open configuration file");
	if (file.peek() == ifstream::traits_type::eof())
		throw invalid_argument("configuration file is empty");
}

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