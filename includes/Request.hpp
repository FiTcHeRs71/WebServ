#ifndef REQUEST_HPP
# define REQUEST_HPP

# include "ServerConfig.hpp"
# include <iostream>
# include <cstddef>
# include <map>
# include <string>
# include <cctype>
# include <sstream>
# include "Default.hpp"

using namespace std;


enum EParseResult
{
	REQ_INCOMPLETE,    //< il mqnaue des octets, rappeler Feed
	REQ_COMPLETE,     //< reqiete entiere disponible
	REQ_ERROR        //< malformee, getErrorCode() donne le status a renvoyer
};

enum EParseState
{
	ST_REQUEST_LINE,
	ST_HEADERS,
	ST_BODY,
	ST_CHUNK_SIZE,
	ST_CHUNK_TRAILER,
	ST_CHUNK_DATA,
	ST_CHUNK_CRLF,
	ST_DONE,
	ST_ERROR
};

class ConfigParser;

/**
 * @brief Represente une requete HTTP recue par le serveur.
 *
 * Accumule les octets recus sur le socket et les parse
 * progressivement (ligne de requete, en-tetes, corps).
 */
class Request
{
	private:

		string				_Raw;		//<accumulateur, consome au fur et a mesure
		string				_Method;
		string				_Path;		//< URI decodee, sans query string
		string				_Query;
		string				_Version;
		string				_Body;
		EParseState			_State;
		map<string, string>	_Header;		//<cles en minuscules
		int					_ErrorCode;		//< 0 tant que tout vas bien
		int					_RequestOctetsSize;
		int					_HeadersOctetsSize;
		size_t				_MaxBodySize;        ///< limite applicable, 0 = illimite (conf)
		size_t				_ContentLength;      ///< longueur annoncee par le header
		bool				_HasContentLength;   ///< le header etait-il present ?
		const ServerConfig*	_Srv;                ///< pose par B, sert a Resolve()
		const ConfigParser*	_Config;
		size_t				_GroupIndex;
		bool				_IsChunked;
		size_t				_CurrentChunkSize;		///< taille du chunk en cours de lecture
		size_t				_CurrentChunkRead;		///< octets deja lus de ce chunk

	public:

	/*===Canonical Form===*/
	Request(void);
	~Request(void);
	Request(const Request& to_copy);
	Request &operator=(const Request& src);

	/*===Getters & Setters===*/
	const string&		getMethod() const;
	const string&		getPath() const;
	const string&		getQuery() const;
	const string&		getVersion() const;
	const int&			getErrorCode() const;
	string				getHeader(const string& key) const;
	bool				setUpContentLength();
	const string&		getBody() const;
	void				SetServerConfig(const ServerConfig *srv);
	map<string, string>	getHeaders(void) const;
	void				SetConnectionContext(const ConfigParser *config, size_t group_index);
	const ServerConfig	*getServerConfig() const;

	/*===Member Function===*/
	EParseResult		Feed(const char *data, size_t n);
	void				reset();
	bool				setUpMethod();
	bool				setUpPath();
	bool				setUpVersion();
	bool				findRequestLine(int n);
	bool				findHeaders(int n);
	bool				expandEncodingUrl();
	void				trimSlash();
	bool				findBody();
	bool				findChunkSize();
	bool				findChunkData();
	bool				findChunkTrailer();
	bool				findChunkCrlf();

	friend ostream& operator<<(ostream& flux, Request& obj);
};

void					trim(string& s);
void					MyToLower(string& key);
bool					isHexa(char c, char d);
char					convertToHexa(char c,char d);

#endif /*REQUEST_HPP*/
