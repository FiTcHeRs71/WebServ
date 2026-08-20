#include "../../includes/http.hpp"

Response	HandleRequest(const Request &request,
					const ServerConfig &server,
					const Connection &connection)
{
	Response	response;
	response.SetStatus(200);
	response.SetBody("Hello from HandleRequest\n");
	return (response);
}