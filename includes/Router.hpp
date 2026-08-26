#ifndef ROOTER_HPP
# define ROOTER_HPP

# include "Response.hpp"
# include "Connection.hpp"
# include "ServerConfig.hpp"
# include "Config.hpp"
# include "Response.hpp"
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>

class Router
{
	private:

	map<string, string>	_mime;

	public:

	/*===Canonical Form===*/
	Router(void);
	~Router(void);
	Router(const Router& to_copy);
	Router &operator=(const Router& src);

	/*===Setters and getters Form===*/

	/*===Methods===*/
	Response	Rooter(const Request &request,
					const ServerConfig &server,
					const Connection &connection);
};

#endif