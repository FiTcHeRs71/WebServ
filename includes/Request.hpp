#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <iostream>
# include <cstddef>
# include <map>

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
	ST_DONE,
	ST_BODY,
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

	protected:

	public:

	/*===Canonical Form===*/
	Request(void);
	~Request(void);
	Request(const Request& to_copy);
	Request &operator=(const Request& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/
	EParseResult	Feed(const char *data, size_t n);
};

#endif /*REQUEST_HPP*/
