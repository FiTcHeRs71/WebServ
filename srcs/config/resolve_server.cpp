#include "../../includes/Config.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <string>
#include <vector>
#include <algorithm>

using namespace std;

static string	to_lower(const string &src)
{
	string	out(src);

	for (size_t i = 0; i < out.size(); i++)
		out[i] = static_cast<char>(tolower(static_cast<unsigned char>(out[i])));
	return (out);
}

static string	split_host_port(const string &host_header)
{
	size_t flag = host_header.rfind(':');
	
	if (flag == string::npos || !is_all_digits(host_header.substr(flag + 1)))
		return (host_header);
	return (host_header.substr(0, flag));
}

const ServerConfig	&SelectServer(const ConfigParser &config, size_t group_index, const string &host_header)
{
	const	TAddrPortGroup	&group = config.getAddrPorts()[group_index];
	string	host_to_reach = split_host_port(host_header);

	for(size_t i = 0; i < group.ServerIndexes.size(); i++)
	{
		const ServerConfig		&srv = config.getServers()[group.ServerIndexes[i]];
		const vector<string>	names = srv.getServerNames();

		for (size_t j =- 0; j < names.size(); j++)
		{
			if (to_lower(names[j]) == host_to_reach)
				return (srv);
		}
	}
	return (config.getServers()[group.DefaultIndex]);
}