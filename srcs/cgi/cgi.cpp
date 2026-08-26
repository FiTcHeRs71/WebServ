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

/**
 * @brief Ajouter une nouvelle valeur a l'environnement cgi.
 *
 * @param storage L'environnement ou ajouter la valeur.
 * @param key Le nom ou assigner la valeur.
 * @param value La valeur a assigner.
 * @return Le nom du script.
*/
void	addEnv(vector<string>& storage, const string &key, const string &value)
{
	storage.push_back(key + "=" + value);
}

/**
 * @brief Trouver le nom du script.
 *
 * @return Le nom du script.
*/
string findScriptName(const Request &request, const LocationConfig &location)
{
	const string	&path = request.getPath();
	const string	&ext = location.getExt();
	size_t			idx = path.find(ext);

	if (idx == string::npos)
		return (path);
	return(path.substr(0, idx + ext.size()));
}

/**
 * @brief Trouver les infomations contenant le chemin vers le script.
 *
 * @return Le chemin vers le script.
*/
string findPathInfo(const Request &request, const LocationConfig &location)
{
	const string	&path = request.getPath();
	const string	&ext = location.getExt();
	size_t			idx = path.find(ext);

	if (idx == string::npos)
		return ("");
	return(path.substr(idx + ext.size()));
}

/**
 * @brief Converti un vecteur de string en tableau de char*.
 *
 * @param strorage Le vecteur.
 * @param envp Le tableau contenant les valeurs du vecteur.
 * @return Le tableau contenant l'env final.
*/
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

/**
 * @brief Transforme les headers dans le bon format de l'env.
 *
 * On prend le nom du header puis on ajoute "HTTP_" au debut du string,
 * les '-' deviennent '_' et tous les caracteres deviennent majuscule.
 *
 * @param name Le nom du header.
 * @param meta Name mis en majuscule avec des '_'.
 * @return "HTTP_" joint au nom transforme.
*/
std::string	header_to_meta(const std::string &name)	///< "Accept-Language" -> "HTTP_ACCEPT_LANGUAGE"
{
	string	meta;
	if (name.empty())
		return ("");
	for (size_t i = 0; i < name.length(); i++)
	{
		if (name[i] == '-')
		{
			meta += '_';
			continue ;
		}
		meta += static_cast<char>(toupper(static_cast<unsigned char>(name[i])));
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
					const ServerConfig &server, const Connection &connection,
					const ConfigParser &config, const string &script_path)
{
	vector<string>	storage;
	vector<string>	serverNames = server.getServerNames();
	string	path_translated;
	ostringstream	ss;
	ss << config.getAddrPorts()[connection.getGroupIndex()].Port;

	addEnv(storage, "REQUEST_METHOD", request.getMethod());
	addEnv(storage, "SCRIPT_NAME", findScriptName(request, location));
	addEnv(storage, "SCRIPT_FILENAME", script_path);
	addEnv(storage, "PATH_INFO", findPathInfo(request, location));
	if (findPathInfo(request, location).empty())
		path_translated = "";
	else
		path_translated = location.getRoot() + findPathInfo(request, location);
	addEnv(storage, "PATH_TRANSLATED", path_translated);
	addEnv(storage, "QUERY_STRING", request.getQuery());
	addEnv(storage, "CONTENT_LENGTH", request.getHeader("content-length"));
	addEnv(storage, "CONTENT_TYPE", request.getHeader("content-type"));
	addEnv(storage, "SERVER_PROTOCOL", "HTTP/1.1");
	addEnv(storage, "SERVER_SOFTWARE", "webserv/1.0");
	if (!serverNames.empty())
		addEnv(storage, "SERVER_NAME", serverNames[0]);
	else
		addEnv(storage, "SERVER_NAME", request.getHeader("host"));
	addEnv(storage, "SERVER_PORT", ss.str());
	addEnv(storage, "GATEWAY_INTERFACE", "CGI/1.1");
	addEnv(storage, "REMOTE_ADDR", connection.getIpV4());
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

// srcs/cgi/cgi.cpp
/**
 * @brief Convertit la sortie brute d'un CGI en Response prete a serialiser.
 *
 * Separe headers CGI et corps sur le premier \r\n\r\n (ou \n\n), interprete
 * Status: et Location:, recalcule systematiquement Content-Length.
 *
 * @param raw La sortie complete du script, lue jusqu'a EOF.
 * @param out La reponse a remplir.
 * @return true si la sortie etait exploitable, false -> l'appelant fait un 502.
*/
bool	parse_cgi_output(const std::string &raw, Response &out){
	(void)out;
	cout << raw << endl;
}
