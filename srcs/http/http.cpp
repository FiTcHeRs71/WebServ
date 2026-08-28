#include "../../includes/http.hpp"
#include "../../includes/Router.hpp"

/**
 * @brief Guichet HTTP unique (S-02). Delegue au Router (C-06).
 *
 * @param request Requete complete (REQ_COMPLETE).
 * @param server Vhost selectionne.
 * @param connection Connexion cliente (non-const depuis B-07).
 * @return Response prete pour Serialize / QueueOutput.
 */
Response	HandleRequest(const Request &request,
					const ServerConfig &server,
					Connection &connection)
{
	return (Router(request, server, connection));
}
