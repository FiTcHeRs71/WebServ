#include "../../includes/Request.hpp"

	/*===Canonical Form===*/
Request::Request(void) : _State(ST_REQUEST_LINE),
						 _ErrorCode(0) {}

Request::~Request(void) {}

Request::Request(const Request& to_copy) :
	_Raw(to_copy._Raw),
	_Method(to_copy._Method),
	_Path(to_copy._Path),
	_Query(to_copy._Query),
	_Version(to_copy._Version),
	_Body(to_copy._Body),
	_State(to_copy._State),
	_Header(to_copy._Header),
	_ErrorCode(to_copy._ErrorCode) {}

Request	&Request::operator=(const Request& src)
{
	if (this != &src)
	{
		this->_Raw = src._Raw;
		this->_Method = src._Method;
		this->_Path = src._Path;
		this->_Query = src._Query;
		this->_Version = src._Version;
		this->_Body = src._Body;
		this->_State = src._State;
		this->_Header = src._Header;
		this->_ErrorCode = src._ErrorCode;
	}
	return (*this);
}

	/*===Getters & Setters===*/


	/*===Member Function===*/


	/*
		POST /upload?type=image HTTP/1.1\r\n     ← ST_REQUEST_LINE (UNE seule ligne, 3 tokens)
		↑        ↑           ↑
		_Method  cible       _Version
				(→ _Path="/upload", _Query="type=image")

		Host: localhost:8080\r\n                 ← ST_HEADERS (une ligne = une entree
		Content-Type: text/plain\r\n                dans _Header, repete tant qu'il y a
		Content-Length: 11\r\n                      des lignes non-vides)

		\r\n                                     ← ligne VIDE = fin des headers,
													signal de transition ST_HEADERS -> ST_BODY

		Hello world                              ← ST_BODY (Content-Length octets a lire,
													hors scope de ton ticket C-01 si celui-ci
													se limite a request-line + headers
	*/

	/**
	 * @brief Alimente le parseur de requete avec un nouveau bloc de donnees.
	 * @param data Pointeur vers les octets recus (non necessairement termines par un caractere nul).
	 * @param n    Nombre d'octets valides dans data.
	 * @return Le statut du parsing (a definir : ex. -1 erreur, 0 incomplet, 1 termine).
	 */
EParseResult	Request::Feed(const char *data, size_t n)
{
	this->_Raw += string(data, n);
	if (this->_State == ST_REQUEST_LINE)
		if (!findRequestLine())
			return (REQ_INCOMPLETE);
	if (this->_State == ST_HEADERS)
		findHeaders();

}

// Methodes pour la requete http
bool Request::findRequestLine(){
	size_t pos = this->_Raw.find("\r\n");
	if (pos == string::npos)
		return false;
	if (!setUpMethod())
		return false;
	if (!setUpPath())
		return false;
	if (!setUpVersion())
		return false;
	return true;
}


bool	Request::setUpMethod(){
	size_t pos = this->_Raw.find_first_of(" ");
	if (pos == string::npos)
		return false;
	this->_Method = this->_Raw.substr(0, pos);
	this->_Raw.erase(0, pos + 1);
	if (this->_Method != "GET" && this->_Method != "POST" && this->_Method != "DELETE"){
		this->_ErrorCode = 1;
		this->_State = ST_ERROR;
		return false;
	}
	return true;
}

bool	Request::setUpPath(){
	size_t pos = this->_Raw.find_first_of(" ");
	if (pos == string::npos)
		return false;
	size_t posQuery = this->_Raw.find_first_of("?");
	if (posQuery == string::npos){
		this->_Path = this->_Raw.substr(0, pos);
		this->_Raw.erase(0, pos + 1);
	}
	else{
		this->_Path = this->_Raw.substr(0, posQuery);
		this->_Query = this->_Raw.substr(posQuery + 1, pos -posQuery);
		this->_Raw.erase(0, pos + 1);
		// Parsing a faire sur le path et le Query
	}
	return true;
}

bool	Request::setUpVersion(){
	size_t pos = this->_Raw.find("\r\n");
	if (pos == string::npos)
		return false;
	this->_Version = this->_Raw.substr(0, pos);
	this->_Raw.erase(0, pos + 1);
	if (this->_Version != "HTTP/1.0" && this->_Version != "HTTP/1.1")
		return false;
	this->_State = ST_BODY;
	return true;
}

bool	Request::findHeaders(){
	size_t pos = this->_Raw.find("\r\n\r\n");
	if (pos == string::npos)
		return false;
	while(true){
		size_t delPos = this->_Raw.find(":");
		if (delPos == string::npos)
			break;

	}
}

/**
 * @brief Fonction permettant de trim les espaes de debut det de fin
 *
 * @param s
 */
void trim(string& s){
	size_t end = s.find_last_of(" \t");
	if (end == string::npos)
		return ;
	s.erase(0, end);
	size_t first = s.find_first_of(" \t");
	if (first == string::npos)
		return;
	s.erase(first + 1);
}
