#include "../../includes/http.hpp"
#include "../../includes/Router.hpp"

Response	HandleRequest(const Request &request,
					const ServerConfig &server,
					Connection &connection)
{
	return (Router(request, server, connection));
}
