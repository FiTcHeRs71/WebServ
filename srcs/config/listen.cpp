# include "../../includes/ServerConfig.hpp"
#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <cstring>
#include <sstream>

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
/**
 * @brief Resout une adresse de directive listen en une ou plusieurs IPv4.
 *
 * "" et "*" designent toutes les interfaces et donnent "0.0.0.0" sans appel
 * systeme. Une IPv4 litterale est validee puis renvoyee telle quelle. Un
 * hostname est resolu par getaddrinfo, une seule fois, ici, au parsing :
 * jamais pendant le service.
 *
 * @param host La partie adresse rendue par split_addr_port().
 * @param out Recoit les IPv4 resolues, dans l'ordre rendu par getaddrinfo.
 * @return void + throw si le host ne resout vers aucune IPv4.
*/
void  resolve_host(const string &host, vector<string> &out)
{
	struct addrinfo	hints;
	struct addrinfo	*res;
	struct addrinfo	*it;
	int				status;

	out.clear();

	if (host.empty() || host == "*")
	{
		out.push_back("0.0.0.0");
		return;
	}
	else if (is_valid_ipv4(host))
	{
		out.push_back(host);
		return;
	}

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	status = getaddrinfo(host.c_str(), NULL, &hints, &res);
	if (status != 0)
		throw runtime_error(host + ": " + gai_strerror(status));

	for (it = res; it != NULL; it = it->ai_next)
	{
		const struct sockaddr_in	*sin;
		uint32_t					ip;
		ostringstream				oss;
		string						ipv4;

		sin = reinterpret_cast<const struct sockaddr_in*>(it->ai_addr);
		ip = ntohl(sin->sin_addr.s_addr);
		oss << static_cast<int>((ip >> 24) & 0xFF) << "."
			<< static_cast<int>((ip >> 16) & 0xFF) << "."
			<< static_cast<int>((ip >> 8) & 0xFF) << "."
			<< static_cast<int>((ip & 0xFF));
		ipv4 = oss.str();
		if (find(out.begin(), out.end(), ipv4) == out.end())
			out.push_back(ipv4);
	}
	freeaddrinfo(res);
	if (out.empty())
		throw runtime_error(host + " does not resolve any IPv4");
}
