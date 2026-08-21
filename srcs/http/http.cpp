#include "../../includes/http.hpp"

Response	HandleRequest(const Request &request,
					const ServerConfig &server)
{
	Response	response;
	response.SetStatus(200);
	response.SetBody("Hello from HandleRequest\n");
	return (response);
}