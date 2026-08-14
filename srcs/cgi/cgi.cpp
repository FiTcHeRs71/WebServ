#include "../../includes/CgiProcess.hpp"
#include "../../includes/ServerConfig.hpp"
#include <vector>
#include <string>

void	addEnv(vector<string>& storage, const string &key, const string &value)
{
	storage.push_back(key + "=" + value);
}

string findScriptName(const Request &request, const LocationConfig &location)
{
	string	Path = request.getPath();
	string	Ext = location.getExt();

	size_t idx = Path.find(Ext);
	string	ScriptName;
	ScriptName.substr(0, idx + Ext.size() - 1);
	return(ScriptName);
}

string findPathInfo(const Request &request, const LocationConfig &location)
{
	string	Path = request.getPath();
	string	Ext = location.getExt();

	size_t idx = Path.find(Ext);
	string	PathInfo;
	PathInfo.substr(0, idx + Ext.size());
	return (PathInfo);
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
					const ServerConfig &server)
{
	vector<string>	storage;
	vector<string>	serverNames = server.getServerNames();
	string		PathTran = server.build_path(location, findPathInfo(request, location));

	addEnv(storage, "REQUEST_METHOD", request.getMethod());
	addEnv(storage, "SCRIPT_NAME", findScriptName(request, location));
	addEnv(storage, "SCRIPT_FILENAME", server.build_path(location, findScriptName(request, location)));
	addEnv(storage, "PATH_INFO", findPathInfo(request, location));
	addEnv(storage, "PATH_TRANSLATED", PathTran);
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