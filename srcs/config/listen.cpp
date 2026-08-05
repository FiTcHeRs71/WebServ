# include "../../includes/ServerConfig.hpp"
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

TListenConfig::TListenConfig(void)
	: Host("")
	,Port(0)
	,IsDefaultServer(false)
{}

TListenConfig::TListenConfig(const string &host, int port, bool is_default_server)
	:Host(host)
	,Port(port)
	,IsDefaultServer(is_default_server)
{}

bool	operator==(const TListenConfig &Listen_a, const TListenConfig &Listen_b)
{
	return (Listen_a.Host == Listen_b.Host && Listen_a.Port == Listen_b.Port);
}

/**
 * @brief Separe la partie adresse de la partie port d'un token listen.
 *
 * Regle NGINX : un token entierement numerique est un port, sinon une adresse.
 * Les deux sorties peuvent etre vides : addr vide = toutes les interfaces,
 * port vide = port implicite. C'est parse_listen_directive() qui applique
 * ces defauts, pas cette fonction.
 *
 * @param token Le premier token de la directive listen.
 * @param addr Recoit la partie adresse, vide si absente.
 * @param port Recoit la partie port, vide si absente.
 * @return void + throw sur token malforme ou forme non supportee.
*/
void	split_addr_port(const string &token, string &addr, string &port)
{
	size_t	separator;

	addr.clear();
	port.clear();

	if (token.empty())
		throw runtime_error("Invalid listen without arguments");
	if (token[0] == '[')
		throw runtime_error("IPv6 is not supported by webserv");
	if (token.compare(0, 5, "unix:") == 0)
		throw runtime_error("unix sockets are not supported by webserv");

	separator = token.find(':');

	if (separator == string::npos)
	{
		if (is_all_digits(token))
			port = token;
		else
			addr = token;
	}
	else
	{
		if (token.find_first_of(':', separator + 1) != string::npos)
			throw runtime_error("Invalid argument for <listen> key");
		addr = token.substr(0, separator);
		port = token.substr(separator + 1);
		if (addr.empty() || port.empty())
			throw runtime_error("Missing argument for <listen> key");
	}
}

vector<TListenConfig>	parse_listen_directive(const vector<string> &tokens)
{
	vector<TListenConfig>	Listens;
	(void)tokens;
	return (Listens);
}