#include "../../includes/Router.hpp"
#include <fcntl.h>
#include <string>

Router::Router(void)
{
	_mime[".html"] = "text/html";
	_mime[".htm"] = "text/html";
	_mime[".css"] = "text/css";
	_mime[".js"] = "text/javascript";
	_mime[".json"] = "application/json";
	_mime[".png"] = "image/png";
	_mime[".jpg"] = "image/jpeg";
	_mime[".jpeg"] = "image/jpeg";
	_mime[".txt"] = "txt/plain";
	_mime[".gif"] = "image/gif";
	_mime[".svg"] = "image/svg+xml";
	_mime[".ico"] = "image/x-icon";
	_mime[".pdf"] = "application/pdf";
	_mime[".mp4"] = "video/mp4";

	//cout << "Router default constructor called" << endl;
}

Router::~Router(void)
{
	//cout << "Router default destructor called" << endl;
}
Router::Router(const Router& to_copy)
	:_mime(to_copy._mime)
{
	//cout << "Router copy constructor called" << endl;
}

Router &Router::operator=(const Router& src)
{
	if (this != &src)
	{
		this->_mime = src._mime;
	}
	return(*this);
}

static string	getKey(string file)
{
	size_t idx = file.find(".");
	string res = file.substr(idx, string::npos);
	return(res);
}

Response	Router::Rooter(const Request &request,
					const ServerConfig &server,
					const Connection &connection)
{
	Response	res;
	const LocationConfig	*loc = server.Resolve(request.getPath());
	if (!loc)
		return(Response::BuildError(404, server));
	string	file = server.build_path(*loc, request.getPath());
	if (!file.empty())
	{
		struct stat statbuf;
		if (stat(file.c_str(), &statbuf) < 0)
			return(Response::BuildError(404, server));
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
		map<string, string>::const_iterator it = _mime.find(getKey(file));
		res.SetStatus(200);
		if (n == 0 && body.empty())
			res.SetBody("");
		else
			res.SetBody(body);
		if (it == _mime.end())
			res.SetHeader("Content-Type", "application/octet-s");
		res.SetHeader("Content-Type", it->second);
		return (res);
	}
	else
		return(Response::BuildError(500, server));
}