#include "../../includes/Response.hpp"

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
		case 200: return "OK";
		case 301: return "Moved Permanently";
		case 302: return "Found";
		case 400: return "Bad Request";
		case 403: return "Forbidden";
		case 404: return "Not Found";
		case 405: return "Method Not Allowed";
		case 408: return "Request Timeout";
		case 411: return "Length Required";
		case 413: return "Payload Too Large";
		case 414: return "URI Too Long";
		case 431: return "Request Header Fields Too Large";
		case 500: return "Internal Server Error";
		case 501: return "Not Implemented";
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