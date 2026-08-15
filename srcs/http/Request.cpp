#include "../../includes/Request.hpp"

	/*===Canonical Form===*/
Request::Request(void) : _State(ST_REQUEST_LINE),
						 _ErrorCode(0),
						 _RequestOctetsSize(0),
						 _HeadersOctetsSize(0) {}

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
	_ErrorCode(to_copy._ErrorCode),
	_RequestOctetsSize(to_copy._RequestOctetsSize),
	_HeadersOctetsSize(to_copy._HeadersOctetsSize) {}

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
		this->_RequestOctetsSize = src._RequestOctetsSize;
		this->_HeadersOctetsSize = src._HeadersOctetsSize;
	}
	return (*this);
}


	/* === === === IMPLEMENTATIONS D'UNE REQUETE === === ===
		POST /upload?type=image HTTP/1.1\r\n     ← ST_REQUEST_LINE (UNE seule ligne, 3 tokens)
		↑        ↑           ↑
		_Method  cible       _Version
				(→ _Path="/upload", _Query="type=image")

		Host: localhost:8080\r\n                 ← ST_HEADERS (une ligne = une entree
		Content-Type: text/plain\r\n                dans _Header, repete tant qu'il y a
		Content-Length: 11\r\n                      des lignes non-vides)

		\r\n                                     ← ligne VIDE = fin des headers,
													signal de transition ST_HEADERS -> ST_BODY

		Hello world                              ← ST_BODY (Content-Length octets a lire)

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
		if (!findRequestLine(n))
			if (this->_State != ST_ERROR)
				return (REQ_INCOMPLETE);
	if (this->_State == ST_HEADERS)
		if (!findHeaders(n))
			if (this->_State != ST_ERROR)
				return (REQ_INCOMPLETE);
	// if (this->_State == ST_BODY)
	// 	if (!findBody())					///< (C-02)
	// 		return (REQ_INCOMPLETE);
	if (this->_State == ST_ERROR)
		return (REQ_ERROR);
	if (this->_State == ST_DONE){
		this->reset();			///< Il peut rester un reste de la prochaine requete dans _Raw
		return (REQ_COMPLETE);	///< donc REQ est incomplete, a checker si cest dans poll qu'on doit reset
	}							///< des qu'une requette est completee
	return (REQ_INCOMPLETE);
}

/**
 * @brief Fonction pour split la requestline <METHOD SP URI SP HTTP/1.1 CRLF>
 *
 * @return true si tout est valide
 * @return false si erreur lors du parsing
 */
bool Request::findRequestLine(int n){
	this->_RequestOctetsSize += n;
	if (this->_RequestOctetsSize > REQUESTMAXSIZE){
		this->_ErrorCode = 414;
		this->_State = ST_ERROR;
		cerr << "Error :" << this->_ErrorCode << ": URI Too Long" << endl;
		return false;
	}
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
	if (!this->_Raw.empty() && this->_Raw[0] == ' '){
		this->_ErrorCode = 400;
		this->_State = ST_ERROR;
		cerr << "Error :" << this->_ErrorCode << ": Bad Request" << endl;
		return false;
	}
	if (this->_Method != "GET" && this->_Method != "POST" && this->_Method != "DELETE"){
		this->_ErrorCode = 405;
		this->_State = ST_ERROR;
		cerr << "Error :" << this->_ErrorCode << ": Method Not Allowed" << endl;
		return false;
	}
	return true;
}

/**
 * @brief Permet de soustraire le path et le query s'il y en a un,
 * parsing et expansion du path
 *
 * @return true tout es ok path valide avec expansion valide + query ou false erreur lors du parsing
 */
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
	}
	if (this->_Path[0] != '/'){
		this->_ErrorCode = 400;
		this->_State = ST_ERROR;
		cerr << "Error :" << this->_ErrorCode << ": Bad Request" << endl;
		return false;
	}
	if (!expandEncodingUrl())
		return false;
	trimSlash();
	this->_Raw.erase(0, pos + 1);
	if (!this->_Raw.empty() && this->_Raw[0] == ' '){
		this->_ErrorCode = 400;
		this->_State = ST_ERROR;
		cerr << "Error :" << this->_ErrorCode << ": Bad Request" << endl;
		return false;
	}
	return true;
}

/**
 * @brief Permet d'extraire la version de la requette
 * Les formats http/1.0 et 1.1 seulement sont pris en compte
 *
 * @return true si la version est juste
 * @return false si la version n'est pas pris en compte
 *
 */
bool	Request::setUpVersion(){
	size_t pos = this->_Raw.find("\r\n");
	if (pos == string::npos)
		return false;
	this->_Version = this->_Raw.substr(0, pos);
	this->_Raw.erase(0, pos + 2);
	if (this->_Version != "HTTP/1.0" && this->_Version != "HTTP/1.1"){
		this->_State = ST_ERROR;
		this->_ErrorCode = 505;
		cerr << "Error: " << this->_ErrorCode << ": HTTP Version Not Supported" << endl;
		return false;
	}
	this->_State = ST_HEADERS;
	return true;
}

/**
 * @brief Fonction permetant de parser le header
 *
 * @return true si le header est correctement structurer
 * @return false si erreur trouver
 */
bool	Request::findHeaders(int n){
	this->_HeadersOctetsSize += n;
	if (this->_HeadersOctetsSize > REQUESTMAXSIZE){
		this->_State = ST_ERROR;
		this->_ErrorCode = 431;
		cerr << "Error: " << this->_ErrorCode << ": Request Header Fields Too Large" << endl;
		return false;
	}
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
			this->_ErrorCode = 400;
			cerr << "Error: " << this->_ErrorCode << ": Bad Request" << endl;
			return false;
		}
		string key = line.substr(0, delPos);
		string value = line.substr(delPos + 1);
		trim(value);
		trim(key);
		MyToLower(key);
		map<string, string>::iterator it = this->_Header.find(key);
		if (it == this->_Header.end())
			this->_Header.insert(make_pair(key, value));
		else{
			if (key == "host"){
				this->_State = ST_ERROR;
				this->_ErrorCode = 400;
				cerr << "Error: " << this->_ErrorCode << ": Bad Request" << endl;
				return false;
			}
			else{
				it->second += ", " + value;
			}
		}
	}
	if (this->_Header.size() > MAXHEADERS){
		this->_State = ST_ERROR;
		this->_ErrorCode = 431;
		cerr << "Error: " << this->_ErrorCode << ": Request Header Fields Too Large" << endl;
		return false;
	}
	this->_Raw.erase(0, pos + 4);
	map<string, string>::iterator it = this->_Header.find("content-length");
	if (it == this->_Header.end())
		this->_State = ST_DONE;
	else
		this->_State = ST_BODY;
	return true;
}

/**
 * @brief Permet de decoder les url encoding
 *
 * @return true
 * @return false
 */
bool	Request::expandEncodingUrl(){
	string tmp;
	for(size_t i = 0; i < this->_Path.size(); i++){
		if (this->_Path[i] != '%')
			tmp.push_back(this->_Path[i]);
		else if (this->_Path[i] == '%' && this->_Path[i + 1] != '\0' && this->_Path[i + 2] != '\0'){
			if (!isHexa(_Path[i + 1], _Path[i + 2])){
				this->_ErrorCode = 400;
				this->_State = ST_ERROR;
				cerr << "Error: " << this->_ErrorCode << ": Bad Request" << endl;
				return false;
			}
			tmp.push_back(convertToHexa(_Path[i + 1], _Path[i + 2]));
			i += 2;
		}
	}
	this->_Path.clear();
	this->_Path = tmp;
	return true;
}

void Request::reset(){
	this->_Method.clear();
	this->_Path.clear();
	this->_Query.clear();
	this->_Version.clear();
	this->_Body.clear();
	this->_State = ST_REQUEST_LINE;
	this->_Header.clear();
	this->_ErrorCode = 0;
	this->_HeadersOctetsSize = 0;
	this->_RequestOctetsSize = this->_Raw.size();
}

/**
 * @brief Fonction for trim the multiple slash in the path of the http request
 *
 */
void Request::trimSlash(){
	string tmp;
	for(size_t i = 0; i < this->_Path.size(); i++){
		if (this->_Path[i] == '/' && !tmp.empty() && tmp[tmp.size() - 1] == '/')
			continue;
		else
			tmp.push_back(this->_Path[i]);
	}
	this->_Path.clear();
	this->_Path = tmp;
}


/*===GETTERS===*/
const string& Request::getMethod() const{
	return this->_Method;
}

/**
 * @brief Getter de path, /!\ Ne jamais repasser la string en string_c car '\0' possible
 * dans la string
 *
 * @return const string&
 */
const string& Request::getPath() const{
	return this->_Path;
}

const string& Request::getQuery() const{
	return this->_Query;
}

const string& Request::getVersion() const{
	return this->_Version;
}


const int& Request::getErrorCode() const{
	return this->_ErrorCode;
}

/**
 * @brief Getters de header
 * /!\ Il est retourner par valeur et non par reference
 *
 * @param key le mot cle en minuscule pour recuperer la value du header
 * @return string&
 */
string Request::getHeader(const string& key) const{
	map<string, string>::const_iterator it = this->_Header.find(key);
	if (it != this->_Header.end())
			return (it->second);
	else
		return("");
}


ostream& operator<<(ostream& flux, Request& obj){
	flux << "Raw = " << obj._Raw << endl;
	flux << "Method = " << obj._Method << endl;
	flux << "Path = " << obj._Path << endl;
	flux << "Query = " << obj._Query << endl;
	// flux << "Body = " << obj._Body << endl;			///< (c-02)
	for(map<string, string>::iterator it = obj._Header.begin(); it != obj._Header.end(); it++){
		flux << "Key = " << it->first << ", Value = " << it->second << endl;
	}
	return flux;
}
