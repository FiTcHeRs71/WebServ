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
		if (this->_State == ST_REQUEST_LINE){
			size_t find = this->_Raw.find_first_of("\r\n");
			if (find != string::npos){
				this->_Method = this->_Raw.substr(0, find);
				this->_Raw.erase(find + 2);
				this->_State = ST_HEADERS;
			}
		}

		else if (this->_State == ST_HEADERS){
			size_t find = this->_Raw.find_first_of("\r\n\r\n");
			if (find != string::npos){
		// Token 2 (la "cible", ex: /index.html?foo=bar) → cherche un ? dedans : ce qui précède va dans _Path, ce qui suit (s'il y a un ?) va dans _Query. Pas de ? → tout va dans _Path, _Query reste vide.
				this->_State = ST_BODY;
			}
		}

		else if (this->_State == ST_BODY){
			size_t find = this->_Raw.find_first_of("\0");
			if (find != string::npos){
				this->_Version = this->_Raw.substr(0, find);
				this->_Raw.erase(find + 2);
				this->_State = ST_BODY;
			}
		}

		return (REQ_INCOMPLETE);
	}
