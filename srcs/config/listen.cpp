# include "../../includes/ServerConfig.hpp"
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

vector<TListenConfig>	parse_listen_directive(const vector<string> &tokens)
{
	vector<TListenConfig>	Listens;
	(void)tokens;
	return (Listens);
}