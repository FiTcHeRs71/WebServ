#ifndef HTTP_HPP
# define HTTP_HPP

# include "./Request.hpp"
# include "./Response.hpp"
# include "./Connection.hpp"
# include "./ServerConfig.hpp"
# include <iostream>

Response	HandleRequest(const Request &request,
					const ServerConfig &server,
					const Connection &connection);

#endif