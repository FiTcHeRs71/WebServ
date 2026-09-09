#include "../../includes/Router.hpp"
#include "../../includes/Autoindex.hpp"
#include "../../includes/CgiProcess.hpp"
#include "../../includes/Logger.hpp"
#include <cstddef>
#include <fcntl.h>
#include <fstream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

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
 * @brief Supprime le fichier vise par un DELETE
 *
 * stat() avant unlink() : il distingue les cas d'echec, ce qui evite de lire
 * errno apres l'appel. Jamais de suppression recursive.
 *
 * @param srv Pour BuildError.
 * @param file Chemin disque deja valide par isInsideRoot().
 * @return 204 sans corps ; 404 absent ; 403 dossier, type special, ou unlink refuse.
 */
static Response	handleDelete(const ServerConfig &srv, string file)
{
	struct stat	sb;
	Response response;

	if (stat(file.c_str(), &sb) < 0)
		return (Response::BuildError(404, srv));
	if (S_ISDIR(sb.st_mode))
		return (Response::BuildError(403, srv));
	if (!S_ISREG(sb.st_mode))
		return (Response::BuildError(403, srv));
	if (unlink(file.c_str()) < 0)
		return (Response::BuildError(403, srv));
	else
		response.SetStatus(204);
	return (response);
}

/**
 * @brief Traite un dossier : 301 sans slash final, sinon index puis serveFile.
 *
 * URI sans '/' final -> 301 Location: URI + "/".
 * Avec slash : parcourt loc.getIndex() dans l'ordre. Aucun index trouve ->
 * autoindex on rend le listing, sinon 403.
 *
 * @param request Pour l'URI (slash / Location).
 * @param loc Location qui matche, source de getIndex().
 * @param server Pour BuildError.
 * @param file Chemin disque du dossier (build_path). Un '/' est ajoute si besoin.
 * @return 301, 200 (index ou autoindex), ou 403.
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
				res.SetHeader("Content-Type", "text/html; charset=utf-8");
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
 * @brief Decoupe un body multipart/form-data sur --boundary (RFC 7578, C-10).
 *
 * Le delimiteur dans le corps est "--" + boundary. Le close porte "--" en plus.
 * Pour chaque part : headers jusqu'au double CRLF, puis octets exacts jusqu'au
 * CRLF qui precede le delimiteur suivant (ce CRLF n'appartient pas au fichier).
 * Data peut contenir des NUL : substr + size, jamais strlen.
 *
 * @param body Corps HTTP brut.
 * @param boundary Valeur de boundary=, sans les '--' (quotes deja retirees).
 * @param out Parts dans l'ordre, y compris les champs texte (Filename vide).
 * @return false si body/boundary vides, delimiteur absent, headers ou close manquants.
 */
bool	parse_multipart(const std::string &body, const std::string &boundary,
						vector<TMultipartPart> &out)
{
	if (body.empty() || boundary.empty())
		return false;
	string delimiter = "--" + boundary;
	size_t i = body.find(delimiter);
	if (i == string::npos)
		return false;
	while(i < body.size())
	{
		TMultipartPart	part;
		size_t			hdrsEnd;
		size_t			dataEnd;

		i += delimiter.size();
		if (i + 1 < body.size() && body[i] == '-' && body[i + 1] == '-')
			return true;
		if (i + 1 >= body.size() || body[i] != '\r' || body[i + 1] != '\n')
			return false;
		hdrsEnd = body.find("\r\n\r\n", i);
		if (hdrsEnd == string::npos)
			return false;
		part.Name = findParam(body.substr(i, hdrsEnd - i), "name=");
		part.Filename = findParam(body.substr(i, hdrsEnd - i), "filename=");
		part.ContentType = findParam(body.substr(i, hdrsEnd - i), "Content-Type: ");
		i = hdrsEnd + 4;
		dataEnd = body.find("\r\n" + delimiter, i);
		if (dataEnd == string::npos)
			return false;
		part.Data = body.substr(i, dataEnd - i);
		out.push_back(part);
		i = dataEnd + 2;
	}
	return false;
}

/**
 * @brief Basename seul, apres le dernier '/' ou '\\'. Refuse vide et leading '.'.
 *
 * Neutralise filename="../../etc/passwd" -> "passwd". ".." et ".hidden" -> "".
 * Le caller ignore "" (part texte, ou 400 en POST raw).
 *
 * @param raw Filename client (multipart) ou URI (POST raw).
 * @return Basename ecrivable dans upload_store, ou "".
 */
std::string	sanitize_filename(const std::string &raw)
{
	if (raw.empty())
		return "";
	string basename;
	size_t	slash = raw.rfind('/');
	if (slash == string::npos)
	{
		size_t backslash = raw.rfind('\\');
		if (backslash == string::npos)
			basename = raw;
		else
			basename = raw.substr(backslash + 1);
	}
	else
		basename = raw.substr(slash + 1, raw.size() - slash);
	if (basename.empty())
		return "";
	if (basename[0] == '.')
		return "";
	return basename;
}

/**
 * @brief Aiguillage C-10 : multipart/form-data ou corps brut.
 *
 * Content-Type contenant "multipart/form-data" -> boundary= puis parse_multipart
 * puis uploadMultipart. Sinon tout le body est le fichier (upload()).
 * boundary= absent ou parse fail -> 400.
 *
 * @param request getHeader("content-type") (cles minuscules) + getBody().
 * @param server Pour BuildError.
 * @param location Doit avoir upload_store (filtre deja pose dans Router).
 * @return 201, 400 ou 500.
 */
static Response	handleUpload(const Request &request,
						const ServerConfig &server,
						const LocationConfig &location)
{
	string value = request.getHeader("content-type");
	if (value.find("multipart/form-data") != string::npos)
	{
		size_t idx = value.find("boundary=");
		if (idx == string::npos)
		{
			return (Response::BuildError(400, server));
		}
		idx += 9;
		string boundary;
		if (!findBoundary(value, boundary, idx))
			return (Response::BuildError(400, server));
		vector<TMultipartPart> parts;
		if (!parse_multipart(request.getBody(), boundary, parts))
			return (Response::BuildError(400, server));
		return (uploadMultipart(server, location, parts));
	}
	else
	{
		return (upload(request, server, location));
	}
}

/**
 * @brief Construit la valeur du header Allow a partir des methodes de la location.
 *
 * Concatene loc.getMethods() en une liste separee par ", ""
 * sans virgule terminale : le separateur est pose avant chaque element sauf
 * le premier.
 *
 * @param loc Location resolue dont on liste les methodes autorisees.
 * @return "GET", "DELETE, GET, POST"
 */
static string  buildAllowHeader(const LocationConfig &loc)
{
	string						allow;
	set<string>::const_iterator	it;

	it = loc.getMethods().begin();
	while (it != loc.getMethods().end())
	{
		if (!allow.empty())
			allow += ", ";
		allow += *it;
		++it;
	}
	return (allow);
}

/**
 * @brief  Point d'entree du routage 
 * @brief Traite le cas ou une location contient un header "return",
 * 	renvoie une réponse en fonction du code d'erreur et de sa target.
 *
 * @param loc Pour les codes et target d'erreurs.
 * @param server Pour BuildError.
 * @return Une reponse http dependant du code d'erreur.
 */
Response	serveReturn(const ServerConfig &server, const LocationConfig &loc)
{
	int	code = loc.getReturnCode();
	string	target = loc.getReturnTarget();
	Response res;
	if (code >= 300 && code <= 399 && code != 304 && !target.empty())
	{
		res.SetStatus(code);
		res.SetHeader("Location", target);
		res.SetHeader("Content-type", "text/html");
		ostringstream oss;
		oss << "<html><body><h1>" << code
			<< " " << res.getStatusText()
			<< "</h1><a href=" << target << ">" << target << "</a></body></html>";
		res.SetBody(oss.str());
		return (res);
	}
	else if (!target.empty() && code != 304 && code != 204)
	{
		res.SetStatus(code);
		res.SetBody(target);
		res.SetHeader("Content-type", "text/plain");
		return (res);
	}
	else if (code == 204 || code == 304)
	{
		res.SetStatus(code);
		return (res);
	}
	return (Response::BuildError(code, server));
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
 * @param connection Reserve au CGI (D-06), inutilise pour le statique.
 * @return La Response a serialiser. Statut 0 = CGI demarre, reponse differee.
 */
static Response	dispatch(const Request &request, const ServerConfig &server, Connection &connection)
{
	const LocationConfig	*loc = server.Resolve(request.getPath());
	if (!loc)
		return(Response::BuildError(404, server));
	if (loc->hasReturn())
		return(serveReturn(server, *loc));
	if (loc->getMethods().count(request.getMethod()) == 0)
	{
		Response	response = Response::BuildError(405, server);
		response.SetHeader("Allow", buildAllowHeader(*loc));
		return (response);
	}
	string	file = server.build_path(*loc, request.getPath());
	if (file.empty())
		return (Response::BuildError(500, server));
	else if (!isInsideRoot(loc->getRoot(), file))
		return (Response::BuildError(403, server));
	else if (request.getMethod() == "DELETE")
		return (handleDelete(server, file));
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
	else if (request.getMethod() == "POST")
	{
		if (loc->hasUploadStore())
		{
			return (handleUpload(request, server, *loc));
		}
		else
			return (Response::BuildError(403, server));
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


/**
 * @brief Point d'entree du routage : dispatch() puis une ligne d'access log.
 *
 * Une ligne "METHODE URI -> statut" par requete. Statut 0 = CGI demarre,
 * la reponse est differee : on trace "cgi started".
 *
 * @param request La requete deja parse, path decode.
 * @param server Le ServerConfig choisi par SelectServer.
 * @param connection Reserve au CGI.
 * @return La Response produite par dispatch(), inchangee.
 */
Response	Router(const Request &request, const ServerConfig &server, Connection &connection)
{
	Response		response = dispatch(request, server, connection);
	ostringstream	oss;

	oss << request.getMethod() << " " << request.getPath() << " -> ";
	if (response.getStatus() == 0)
		oss << "cgi started";
	else
		oss << response.getStatus();
	Logger::write("info", oss.str());
	return (response);
}
