#include "../../includes/Request.hpp"

	/*===Canonical Form===*/
Request::Request(void) : _State(ST_REQUEST_LINE),
						 _ErrorCode(0) {}

Request::~Request(void) {}

Request::Request(const Request& to_copy) : _Raw(to_copy._Raw),
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
		La ligne de requête a le format "METHODE ESPACE cible ESPACE HTTP/version" — 3 tokens séparés par des espaces :


		"GET /index.html?foo=bar HTTP/1.1"
		↓         ↓                ↓
		_Method   cible           _Version
		Découpe la ligne sur les espaces → tu dois obtenir exactement 3 tokens (si ce n'est pas le cas : _State = ST_ERROR, requête malformée, REQ_ERROR).
		Token 1 → directement dans _Method.
		Token 2 (la "cible", ex: /index.html?foo=bar) → cherche un ? dedans : ce qui précède va dans _Path, ce qui suit (s'il y a un ?) va dans _Query. Pas de ? → tout va dans _Path, _Query reste vide.
		Token 3 → dans _Version.
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
	// cout << "_Here = [[[" << this->_Raw << "]]]" << endl;
	if (this->_State == ST_REQUEST_LINE)
		if (!this->findMethod() && this->_State != ST_ERROR)
			return	(REQ_INCOMPLETE);
	if (this->_State == ST_HEADERS)
		if (!this->findPath() && this->_State != ST_ERROR)
			return	(REQ_INCOMPLETE);
	if (this->_State == ST_BODY)
		if (!this->findVersion() && this->_State != ST_ERROR)
			return (REQ_COMPLETE);
	if (this->_State == ST_ERROR){
		this->_ErrorCode = 1;
		return (REQ_ERROR);
	}
	else
		return (REQ_INCOMPLETE);
}

bool	Request::findMethod(){
	size_t find = this->_Raw.find_first_of("\r\n");
	if (find != string::npos){
		this->_Method = this->_Raw.substr(0, find);
		cout << "_Method = " << this->_Method << endl;
		if (this->_Method != "GET" && this->_Method != "POST" && this->_Method != "DELETE"){
			this->_State = ST_ERROR;
			return false;
		}
		this->_State = ST_HEADERS;
		this->_Raw.erase(0, find + 2);
		return true;
	}
	else
		return false;
}

bool	Request::findPath(){
	size_t find = this->_Raw.find_first_of("\r\n\r\n");
	if (find != string::npos){
		size_t findQuery = this->_Raw.find("?");
		if (findQuery != string::npos){
			this->_Path = this->_Raw.substr(0, findQuery);
			this->_Query = this->_Raw.substr(findQuery + 1, find - findQuery);
			this->_Raw.erase(0, find + 4);
		}
		else
			this->_Path = this->_Raw.substr(0, find);
		this->_State = ST_BODY;
		cout << "_Path = " << this->_Path << endl << " _Query = " << this->_Query << endl;
		return true;
	}
	else
		return false;
}

bool	Request::findVersion(){
	if (this->_Raw == "HTTP/1.1" || this->_Raw == "HTTP/1.0"){
		this->_Version = this->_Raw;
		this->_State = ST_DONE;
		cout << "_Version = " << this->_Version << endl;
		return true;
	}
	return false;
}

