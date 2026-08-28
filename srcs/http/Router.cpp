#include "../../includes/Router.hpp"
#include <fcntl.h>
#include <string>

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

static string	getKey(string file)
{
	size_t slash = file.rfind("/");
	size_t dot = file.rfind(".");
	if (dot == string::npos || (slash != string::npos && dot < slash))
		return("unknown");
	else
		return(file.substr(dot, string::npos));
}

static void	getMime(Response &res, map<string, string> mime, string path)
{
	map<string, string>::const_iterator it = mime.find(getKey(path));
	if (it == mime.end())
		res.SetHeader("Content-Type", "application/octet-stream");
	else
		res.SetHeader("Content-Type", it->second);
}

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

static Response	serveDir(const Request &request, const LocationConfig &loc,
			const ServerConfig &server, string file)
{
	Response res;

	string str = request.getPath();
	string::iterator it = str.end();
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
				break ;
		}
		if (it1 == index.end())
			return(Response::BuildError(403, server));
		return (serveFile(server, path));
	}
}


/*Savoir si le chemin est absolu ('.') ou non (home/...). Découpe le chemin disque en segments (/ ).
. → ignore. .. → enlève le segment d’avant. S’il n’y a plus rien à enlever, tu es sorti du root → 403.
mettre chaque segment dans un vecteur.¨
si pas de 403, reconstruire le chemin à partir du vecteur en ajoutant de '/' si besoin.*/
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

Response	Router(const Request &request,
				const ServerConfig &server,
				const Connection &connection)
{
	(void)connection;
	Response	res;
	const LocationConfig	*loc = server.Resolve(request.getPath());
	if (!loc)
		return(Response::BuildError(404, server));
	string	file = server.build_path(*loc, request.getPath());
	if (file.empty())
		return (Response::BuildError(500, server));
	else if (!isInsideRoot(loc->getRoot(), file))
		return (Response::BuildError(403, server));
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