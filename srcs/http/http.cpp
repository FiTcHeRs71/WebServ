#include "../../includes/http.hpp"

Response	HandleRequest(const Request &request,
					const ServerConfig &server,
					const Connection &connection)
{
	(void)request;
	(void)server;
	(void)connection;
	Response	response;
	response.SetStatus(200);
	response.SetBody("Hello from HandleRequest\n");
	return (response);
}