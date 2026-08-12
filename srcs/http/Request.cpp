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
		if (!findHeaders())
			return (REQ_INCOMPLETE);
	// if (this->_State == ST_BODY)
	// 	if (!findBody)					///< A Remplir par Ticket (C-02)
	// 		return (REQ_INCOMPLETE);
	if (this->_State == ST_ERROR)
		return (REQ_ERROR);
	return (REQ_INCOMPLETE);

}

/**
 * @brief Fonction pour split la requestline METHOD SP URI SP HTTP/1.1 CRLF
 *
 * @return true si tout est valide
 * @return false si erreur lors du parsing
 */
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

//! A faire , convertir les URL encoding (%20) -> Check % -> check si suivi de 2
//! chiffre hexa -> convertir le hexa en valeur ->
//! et remplacer la nouvelle string grace a str.replace("...")
bool	Request::setUpPath(){
	size_t pos = this->_Raw.find_first_of(" ");
	if (pos == string::npos)
		return false;
	size_t posQuery = this->_Raw.find_first_of("?");
	if (posQuery > pos || posQuery == string::npos){
		this->_Path = this->_Raw.substr(0, pos);
	}
	else{
		this->_Path = this->_Raw.substr(0, posQuery);
		this->_Query = this->_Raw.substr(posQuery + 1, pos - posQuery - 1);
		// Parsing a faire sur le path et le Query
	}
	if (this->_Path[0] != '\\'){
		this->_ErrorCode = 400;
		this->_State = ST_ERROR;
	}
	this->_Raw.erase(0, pos + 1);
	return true;
}

bool	Request::setUpVersion(){
	size_t pos = this->_Raw.find("\r\n");
	if (pos == string::npos)
		return false;
	this->_Version = this->_Raw.substr(0, pos);
	this->_Raw.erase(0, pos + 2);
	if (this->_Version != "HTTP/1.0" && this->_Version != "HTTP/1.1"){
		this->_State = ST_ERROR;
		this->_ErrorCode = 1;
		return false;
	}
	this->_State = ST_HEADERS;
	return true;
}

bool	Request::findHeaders(){
	size_t pos = this->_Raw.find("\r\n\r\n");
	if (pos == string::npos)
		return false;
	stringstream ss(_Raw);
	string line;
	while(getline(ss, line)){
		if (line.empty() || line == "\r")
			break;
		size_t delPos = line.find_first_of(":");
		if (delPos == string::npos){
			this->_State = ST_ERROR;
			this->_ErrorCode = 1;
			return false;
		}
		string key = line.substr(0, delPos);
		string value = line.substr(delPos + 1);
		trim(value);
		trim(key);
		MyToLower(key);
		this->_Header.insert(make_pair(key, value));
	}
	this->_Raw.erase(0, pos + 4);
	this->_State = ST_BODY;
	return true;
}



/*===Fonctions externes===*/
/**
 * @brief Fonction permettant de trim les espaes de debut det de fin
 *
 * @param s
 */
void trim(string& s){
	size_t end = s.find_last_not_of(" \t\r\n");
	if (end == string::npos)
		return ;
	s.erase(end + 1);
	size_t first = s.find_first_not_of(" \t");
	if (first == string::npos)
		return;
	s.erase(0, first);
}

void MyToLower(string& key){
	for(size_t i = 0; i < key.size(); i++){
		key[i] = tolower(key[i]);
	}
}

/*===GETTERS===*/
const string& Request::getMethod() const{
	return this->_Method;
}

const string& Request::getPath() const{
	return this->_Path;
}

const string& Request::getQuery() const{
	return this->_Query;
}

const string& Request::getVersion() const{
	return this->_Version;
}

// const map<string, string>& Request::getHeader(string key) const{
// 	for (map<string, string>::iterator it = this->_Header.begin(); it != this->_Header.end(); it++){
// 		if (it->first == key)
// 			return *it;
// 	}
// }
