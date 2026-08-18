#ifndef REQUEST_HPP
# define REQUEST_HPP

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
	ST_DONE,
	ST_ERROR
};

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

	protected:

	public:

	/*===Canonical Form===*/
	Request(void);
	~Request(void);
	Request(const Request& to_copy);
	Request &operator=(const Request& src);

	/*===Getters & Setters===*/
	const string&	getMethod() const;
	const string&	getPath() const;
	const string&	getQuery() const;
	const string&	getVersion() const;
	const int&		getErrorCode() const;
	string			getHeader(const string& key) const;

	// const string&	getBody() const;		///< (C-02)

	/*===Member Function===*/
	EParseResult	Feed(const char *data, size_t n);
	void			reset();
	bool			setUpMethod();
	bool			setUpPath();
	bool			setUpVersion();
	bool			findRequestLine(int n);
	bool			findHeaders(int n);
	bool			expandEncodingUrl();
	void			trimSlash();
	// bool	findBody(-_r);		///< (C-02)

	friend ostream& operator<<(ostream& flux, Request& obj);
};

void trim(string& s);
void MyToLower(string& key);
bool isHexa(char c, char d);
char convertToHexa(char c,char d);

#endif /*REQUEST_HPP*/
