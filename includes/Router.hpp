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


Response	Router(const Request &request,
				const ServerConfig &server,
				const Connection &connection);

#endif