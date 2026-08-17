#include "../../includes/CgiProcess.hpp"
#include "../../includes/ServerConfig.hpp"
#include "../../includes/Request.hpp"
#include "../../includes/Connection.hpp"
#include "../../includes/Config.hpp"
#include <cctype>
#include <map>
#include <sstream>
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

char	**VectorToChar(vector<string> &storage)
{
	char **envp = new char*[storage.size() + 1];

	for(size_t i = 0; i < storage.size(); i++)
	{
		envp[i] = const_cast<char*>(storage[i].c_str());
	}
	envp[storage.size()] = NULL;
	return (envp);
}

std::string	header_to_meta(const std::string &name)	///< "Accept-Language" -> "HTTP_ACCEPT_LANGUAGE"
{
	string	meta;
	if (name.empty())
		return ("");
	for (size_t i = 0; i < name.length(); i++)
	{
		if (name[i] == '-')
		{
			meta[i] += '_';
			continue ;
		}
		meta[i] += static_cast<char>(toupper(static_cast<unsigned char>(name[i])));
	}
	return("HTTP_" + meta);
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
vector<string>	build_cgi_env(const Request &request, const LocationConfig &location,
					const ServerConfig &server, const Connection &connection, const ConfigParser &config)
{
	vector<string>	storage;
	vector<string>	serverNames = server.getServerNames();
	string		PathTran = server.build_path(location, findPathInfo(request, location));
	ostringstream	ss;
	ss << config.getAddrPorts()[connection.getGroupIndex()].Port;

	addEnv(storage, "REQUEST_METHOD", request.getMethod());
	addEnv(storage, "SCRIPT_NAME", findScriptName(request, location));
	addEnv(storage, "SCRIPT_FILENAME", server.build_path(location, findScriptName(request, location)));
	addEnv(storage, "PATH_INFO", findPathInfo(request, location));
	addEnv(storage, "PATH_TRANSLATED", PathTran);
	addEnv(storage, "QUERY_STRING", request.getQuery());
	addEnv(storage, "CONTENT_LENGTH", request.getHeader("content-length"));
	addEnv(storage, "CONTENT_TYPE", request.getHeader("content-type"));
	addEnv(storage, "SERVER_PROTOCOL", "HTTP/1.1");
	addEnv(storage, "SERVER_SOFTWARE", "webserv/1.0");
	addEnv(storage, "SERVER_NAME", serverNames[0]);
	addEnv(storage, "SERVER_PORT", ss.str());
	addEnv(storage, "GATEWAY_INTERFACE", "CGI/1.1");
	addEnv(storage, "REMOTE_ADDR", connection.ip);
	addEnv(storage, "REDIRECT_STATUS", "200");

	const map<string, string>	headers = request.getHeaders();
	for(map<string, string>::const_iterator it = headers.begin(); it != headers.end(); it++)
	{
		if (it->first == "content-length" || it->first == "content-type")
			continue ;
		addEnv(storage, header_to_meta(it->first), it->second);
	}
	return (storage);
}
