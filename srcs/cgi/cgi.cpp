#include "../../includes/CgiProcess.hpp"
#include "../../includes/ServerConfig.hpp"
#include <vector>
#include <string>

void	addEnv(vector<string>& storage, string &key, string &value)
{
	storage.push_back(key + '=' + value);
}

/**
 * @brief Construit les meta-variables CGI/1.1 de la requete.
 *
 * Les chaines sont conservees dans "storage" : les char* de l'envp rendu
 * pointent dedans et ne survivent pas a sa destruction. Construire AVANT
 * le fork() et garder "storage" vivant jusqu'a l'execve().
 *
 * @param request La requete parsee (C-01).
 * @param location La LocationConfig resolue (A-05).
 * @param storage Recoit les chaines "CLE=valeur".
 * @return Le tableau char** terminé par NULL, a passer a execve().
*/
char	**build_cgi_env(const Request &request, const LocationConfig &location,
					const ServerConfig &server, std::vector<std::string> &storage)
{
	vector<string>	serverNames = server.getServerNames();

	addEnv(storage, "REQUEST_METHOD", request.getMethod());
	addEnv(storage, "SCRIPT_NAME", );
	addEnv(storage, "SCRIPT_FILENAME", );
	addEnv(storage, "PATH_INFO", );
	addEnv(storage, "PATH_TRANSLATED", );
	addEnv(storage, "QUERY_STRING", request.getQuery());
	addEnv(storage, "CONTENT_LENGTH", );
	addEnv(storage, "CONTENT_TYPE", request.getHeader());
	addEnv(storage, "SERVER_PROTOCOL", "HTTP/1.1");
	addEnv(storage, "SERVER_SOFTWARE", "webserv/1.0");
	addEnv(storage, "SERVER_NAME", serverNames[0]);
	addEnv(storage, "SERVER_PORT", );
	addEnv(storage, "GATEWAY_INTERFACE", "CGI/1.1");
	addEnv(storage, "REMOTE_ADDR", );
	addEnv(storage, "REDIRECT_STATUS", "200");
}

std::string	header_to_meta(const std::string &name);	///< "Accept-Language" -> "HTTP_ACCEPT_LANGUAGE"