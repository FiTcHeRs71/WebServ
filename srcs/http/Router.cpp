#include "../../includes/Router.hpp"
#include "../../includes/Autoindex.hpp"
#include "../../includes/CgiProcess.hpp"
#include <fcntl.h>
#include <string>

/**
 * @brief Table MIME extension -> Content-Type, construite une seule fois.
 * @return Reference sur la map statique. Defaut cote appelant : octet-stream.
 */
static map<string, string> &mime_table(void)
{
	static map<string, string> mime;
	if (mime.empty())
	{
		mime[".html"] = "text/html";
		mime[".htm"] = "text/html";
		mime[".css"] = "text/css";
		mime[".js"] = "application/javascript";
		mime[".json"] = "application/json";
		mime[".png"] = "image/png";
		mime[".jpg"] = "image/jpeg";
		mime[".jpeg"] = "image/jpeg";
		mime[".txt"] = "text/plain";
		mime[".gif"] = "image/gif";
		mime[".svg"] = "image/svg+xml";
		mime[".ico"] = "image/x-icon";
		mime[".pdf"] = "application/pdf";
		mime[".mp4"] = "video/mp4";
	}
	return (mime);
}

/**
 * @brief Extraie l'extension du nom de fichier (dernier '.' apres le dernier '/').
 * @param file Chemin disque ou URI.
 * @return ".html", ".png"... ou "unknown" s'il n'y a pas d'extension.
 */
static string	getKey(string file)
{
	size_t slash = file.rfind("/");
	size_t dot = file.rfind(".");
	if (dot == string::npos || (slash != string::npos && dot < slash))
		return("unknown");
	else
		return(file.substr(dot, string::npos));
}

/**
 * @brief Pose Content-Type sur res d'apres l'extension de path.
 *
 * Lookup rate -> application/octet-stream.
 * @param res Reponse a completer.
 * @param mime Table renvoyee par mime_table().
 * @param path Chemin du fichier effectivement servi.
 */
static void	getMime(Response &res, map<string, string> mime, string path)
{
	map<string, string>::const_iterator it = mime.find(getKey(path));
	if (it == mime.end())
		res.SetHeader("Content-Type", "application/octet-stream");
	else
		res.SetHeader("Content-Type", it->second);
}

/**
 * @brief Sert un fichier regulier : open/read binaire, MIME, Content-Length.
 *
 * open/read fail -> 403. Fichier vide -> 200, Content-Length 0.
 * @param server Pour BuildError.
 * @param file Chemin disque deja valide (dans le root, S_ISREG).
 * @return 200 + body, ou BuildError(403).
 */
static Response	serveFile(const ServerConfig &server, string file)
{
	Response res;
	map<string, string> mime = mime_table();
	int fd;
	if ((fd = open(file.c_str(), O_RDONLY)) < 0)
		return(Response::BuildError(403, server));
	char	buf[4096];
	ssize_t	n;
	string	body;
	while ((n = read(fd, buf, sizeof(buf))) > 0)
		body.append(buf, static_cast<size_t>(n));
	close(fd);
	if (n < 0)
		return(Response::BuildError(403, server));
	getMime(res, mime, file);
	res.SetStatus(200);
	if (n == 0 && body.empty())
		res.SetBody("");
	else
		res.SetBody(body);
	return (res);
}

/**
 * @brief Traite un dossier : 301 sans slash final, sinon index puis serveFile.
 *
 * URI sans '/' final -> 301 Location: URI + "/".
 * Avec slash : parcourt loc.getIndex() dans l'ordre. Aucun index -> 403
 * (l'autoindex est C-07).
 * @param request Pour l'URI (slash / Location).
 * @param loc Location qui matche, source de getIndex().
 * @param server Pour BuildError.
 * @param file Chemin disque du dossier (build_path). Un '/' est ajoute si besoin.
 * @return 301, 200 (index), ou 403.
 */
static Response	serveDir(const Request &request, const LocationConfig &loc,
			const ServerConfig &server, string file)
{
	Response res;

	string str = request.getPath();
	string::iterator it = str.end();
	if (str.empty())
		return(Response::BuildError(400, server));
	it--;
	if (*it != '/')
	{
		res.SetStatus(301);
		res.SetHeader("Location", request.getPath() + "/");
		res.SetBody("");
		return (res);
	}
	else
	{
		vector<string> index = loc.getIndex();
		vector<string>::iterator it1 = index.begin();
		string path;
		if (file.at(file.size() - 1) != '/')
			file += "/";
		while (it1 != index.end())
		{
			path = file + *it1;
			struct stat sf;
			if (stat(path.c_str(), &sf) < 0)
				it1++;
			else
				break;
		}
		if (it1 == index.end())
		{
			if (!loc.getAutoIndex())
				return(Response::BuildError(403, server));
			else
			{
				string	body;

				body = build_autoindex(file, request.getPath());
				if (body.empty())
					return(Response::BuildError(403, server));
				res.SetStatus(200);
				res.SetHeader("Content-Type", "text/html");
				res.SetBody(body);
				return (res);
			}
		}
		return (serveFile(server, path));
	}
}


/**
 * @brief Normalise un chemin : collapse des '/', ignore '.', resout '..'.
 *
 * Ne touche pas au disque (pas de realpath). Un '..' alors que la pile est
 * vide pose escaped = true et rend une string vide : on est sorti du point
 * de depart.
 * @param strIn Chemin brut (root ou path disque).
 * @param escaped Out : true si un '..' depasse la racine de strIn.
 * @return Chemin recollé, ou "" si escaped.
 */
static string normalizePath(const string &strIn, bool &escaped)
{
	size_t		i = 0;
	bool		abs = !strIn.empty() && strIn[0] == '/';
	vector<string>	st;

	escaped = false;
	while (i < strIn.size())
	{
		while(i < strIn.size() && strIn[i] == '/')
			i++;
		if(i >= strIn.size())
			break ;
		size_t j = strIn.find('/', i);
		if (j == string::npos)
			j = strIn.size();
		string part = strIn.substr(i, j - i);
		i = j;
		if (part == "." || part.empty())
			continue ;
		else if (part == "..")
		{
			if (st.empty())
			{
				escaped = true;
				return("");
			}
			st.pop_back();
			continue ;
		}
		st.push_back(part);
	}
	string out = abs ? "/" : "";
	for(size_t k = 0; k < st.size(); k++)
	{
		if (!out.empty() && out[out.size() - 1] != '/')
			out += "/";
		out += st[k];
	}
	if (abs && st.empty())
		out = "/";
	return (out);

}

/**
 * @brief true si path, une fois normalise, reste sous root (frontiere '/').
 *
 * escaped ou root vide -> false. Empêche /var/www-evil de matcher /var/www.
 * @param root Root de la location (loc->getRoot()).
 * @param path Chemin disque issu de build_path.
 * @return false -> le caller rend 403.
 */
static bool isInsideRoot(const string &root, const string &path)
{
	bool	escRoot;
	bool	escPath;
	string	nRoot = normalizePath(root, escRoot);
	string	nPath = normalizePath(path, escPath);

	if (escRoot || escPath || nRoot.empty())
		return (false);
	if (nPath == nRoot)
		return (true);
	if (nRoot[nRoot.size() - 1] != '/')
		nRoot += '/';
	return (nPath.size() >= nRoot.size()
		&& nPath.compare(0, nRoot.size(), nRoot) == 0);
	
}

/**
 * @brief true si le chemin de la request correspond a _CgiExt et si _CgiPass n'est pas vide.
 * 
 * @return false -> le caller continue comme un fichier normal.
 */
static bool	isCgi(const Request &request, const LocationConfig &loc)
{
	return (getKey(request.getPath()) == loc.getExt() && !loc.getPass().empty());
}

/**
 * @brief Point d'entree du GET statique (C-06).
 *
 * Resolve la location, traduit l'URI en chemin disque, refuse le path
 * traversal, puis sert un fichier, un index de dossier, ou une redirection
 * 301 /dir -> /dir/. Les erreurs passent par Response::BuildError.
 *
 * @param request La requete deja parse, path %-decode.
 * @param server Le ServerConfig choisi par SelectServer (S-03).
 * @param connection Inutilise pour le statique (reserve CGI / D-06).
 * @return La Response a serialiser, jamais une reponse vide.
 */
Response	Router(const Request &request,
				const ServerConfig &server,
				Connection &connection)
{
	const LocationConfig	*loc = server.Resolve(request.getPath());
	if (!loc)
		return(Response::BuildError(404, server));
	string	file = server.build_path(*loc, request.getPath());
	if (file.empty())
		return (Response::BuildError(500, server));
	else if (!isInsideRoot(loc->getRoot(), file))
		return (Response::BuildError(403, server));
	else if (isCgi(request, *loc))
	{
		CgiProcess		&cgi = connection.getCgi();
		const ConfigParser	*config = request.getConfigParser();
		if (config == NULL)
			return (Response::BuildError(502, server));
		if (!cgi.Start(request, *loc, server, connection, *config, loc->getRoot() + request.getPath()))
			return (Response::BuildError(502, server));
		else
			return (Response());
	}
	else
	{
		struct stat statbuf;
		if (stat(file.c_str(), &statbuf) < 0)
			return(Response::BuildError(404, server));
		if (S_ISDIR(statbuf.st_mode))
		{
			return (serveDir(request, *loc, server, file));
		}
		else if (S_ISREG(statbuf.st_mode))
		{
			return (serveFile(server, file));
		}
		else
			return(Response::BuildError(404, server));
	}
}