#include "../../includes/Response.hpp"
#include <climits>
#include <map>
#include <sstream>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

	/*===Canonical Form===*/
Response::Response(void)
{
	_StatusCode = 0;
	_StatusText = "";
	_Body = "";
	//std::cout << "Response default constructor called" << std::endl;
}

Response::~Response(void)
{
	//std::cout << "Response default destructor called" << std::endl;
}

Response::Response(const Response& to_copy)
	:_StatusCode(to_copy._StatusCode)
	,_StatusText(to_copy._StatusText)
	,_Headers(to_copy._Headers)
	,_Body(to_copy._Body)
{
	//std::cout << "Response copy constructor called" << std::endl;
}

Response	&Response::operator=(const Response& src)
{
	//std::cout << "Response operator assignement(=) constructor called" << std::endl;
	if (this != &src)
	{
		_StatusCode = src._StatusCode;
		_StatusText = src._StatusText;
		_Headers = src._Headers;
		_Body = src._Body;
	}
	return (*this);
}

/*===Getters & Setters===*/
static std::string	StatusText(int code)
{
	switch (code)
	{
		case 100: return "Continue";
		case 101: return "Switching Protocols";
		case 200: return "OK";
		case 201: return "Created";
		case 202: return "Accepted";
		case 203: return "Non-Authoritative Information";
		case 204: return "No Content";
		case 205: return "Reset Content";
		case 206: return "Partial Content";
		case 300: return "Multiple Choices";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 303: return "See Other";
		case 304: return "Not Modified";
		case 305: return "Use Proxy";
		case 307: return "Temporary Redirect";
		case 400: return "Bad Request";
		case 401: return "Unauthorized";
		case 402: return "Payment Required";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 406: return "Not Acceptable";
		case 407: return "Proxy Authentication Required";
		case 408: return "Request Timeout";
		case 409: return "Conflict";
		case 410: return "Gone";
		case 411: return "Length Required";
		case 412: return "Precondition Failed";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 415: return "Unsupported Media Type";
		case 416: return "Range Not Satisfiable";
		case 417: return "Expectation Failed";
		case 426: return "Upgrade Required";
		case 431: return "Request Header Fields Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
		case 502: return "Bad Gateway";
		case 503: return "Service Unavailable";
		case 504: return "Gateway Timeout";
		case 505: return "HTTP Version Not Supported";
		default: return "Unknown";
	}
}

void	Response::SetStatus(int code)
{
	this->_StatusCode = code;
	this->_StatusText = StatusText(code);
}

void	Response::SetHeader(const std::string &key, const std::string &value)
{
	this->_Headers[key] = value;
}

void	Response::SetBody(const std::string &body)
{
	this->_Body = body;
	ostringstream oss;
	oss << body.size();
	SetHeader("Content-Length", oss.str());
}

const std::string	&Response::getBody(void) const
{
	return(this->_Body);
}

int	Response::getStatus(void) const
{
	return(this->_StatusCode);
}

	/*===Member Function===*/

	/**
 * @brief Reset les valeurs de la class Response pour reprendre la prochaine serialization.
 */
void	Response::Reset(void)
{
	_StatusCode = 0;
	_StatusText.clear();
	_Body.clear();
	_Headers.clear();
}

/**
 * @brief Calcule et ajoute aux headers la date en anglais a l'instant T.
 */
void	Response::setDate(void)
{
	char		date[64];
	time_t		now;
	struct tm	*gmt;
	now = time(NULL);
	gmt = gmtime(&now);
	if (gmt != NULL)
	{
		setlocale(LC_TIME, "C");
		strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S GMT", gmt);
		SetHeader("Date", date);
	}
}

/**
 * @brief Serialise la reponse HTTP en une chaine prete a etre envoyee sur le socket.
 * @param out Chaine dans laquelle la reponse serialisee sera ecrite.
 * @return true si la serialisation a reussi, false sinon.
 */
bool	Response::Serialize(std::string &out)
{
	out.clear();
	ostringstream oss;
	oss << _StatusCode;
	out += "HTTP/1.1 " + oss.str() + " " + _StatusText + "\r\n";
	SetHeader("Server", "webserv");
	setDate();
	for (map<string, string>::iterator it = _Headers.begin(); it != _Headers.end(); it++)
	{
		out += it->first + ": "  + it->second  + "\r\n";
	}
	out += "\r\n";
	out += _Body;
	return (true);
}

/**
 * @brief Cree un body de reponse http en fonction de l'erreur.
 */
void	Response::generateBuiltInError(void)
{
	ostringstream oss;
	oss << "<html><body><h1>" << _StatusCode
		<< " " << _StatusText << "</h1></body></html>";
	SetBody(oss.str());
}

/**
 * @brief Lit le fichier d'erreur s'il existe, sinon cree un built in,
 * renvoie le tout comme reponse.
 * @param code Code d'erreur.
 * @param server Server ou se trouve les pages d'erreur.
 * @return Une reponse d'erreur http.
 */
Response	Response::BuildError(int code, const ServerConfig &server)
{
	Response	res;
	res.SetStatus(code);
	res.SetHeader("Content-Type", "text/html");
	const map<int, string>				&ErrorPages = server.getErrorPages();
	map<int, string>::const_iterator	it = ErrorPages.find(code);
	if (it == ErrorPages.end())
	{
		res.generateBuiltInError();
		return res;
	}
	const LocationConfig	*loc = server.Resolve(it->second);
	if(!loc)
	{
		res.generateBuiltInError();
		return res;
	}
	string					ErrorPath = server.build_path(*loc, it->second);
	if(!ErrorPath.empty())
	{
		int	fd = open(ErrorPath.c_str(), O_RDONLY);
		if (fd < 0)
		{
			res.generateBuiltInError();
			return res;
		}
		char	buf[4096];
		ssize_t	n;
		string	body;
		while ((n = read(fd, buf, sizeof(buf))) > 0)
		{
			body.append(buf, static_cast<size_t>(n));
		}
		close(fd);
		if (n < 0 || body.empty())
		{
			res.generateBuiltInError();
			return res;
		}
		else
			res.SetBody(body);
	}
	else
		res.generateBuiltInError();
	return res;
}
