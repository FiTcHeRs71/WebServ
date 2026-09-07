#ifndef RESPONSE_HPP
# define RESPONSE_HPP

#include "ServerConfig.hpp"
# include <iostream>
# include <string>
# include <map>
# include <ctime>
# include <clocale>
# include <sstream>

using namespace std;

/**
 * @brief Represente une reponse HTTP a construire et a envoyer au client.
 *
 * Regroupe le statut, les en-tetes et le corps de la reponse,
 * et sait se serialiser en une chaine prete a etre ecrite sur le socket.
 */
class Response
{
	private:

	int									_StatusCode;	///< 200, 404, 500...
	std::string							_StatusText;	///< "OK", "Not Found"...
	std::map<std::string, std::string>	_Headers;	///< cles telles quelles ("Content-Type")
	std::string							_Body;

	void	setDate(void);

	protected:



	public:

	/*===Canonical Form===*/
	Response(void);
	~Response(void);
	Response(const Response& to_copy);
	Response &operator=(const Response& src);

	/*===Getters & Setters===*/
	void				SetStatus(int code);					///< pose _StatusCode ET _StatusText
	void				SetHeader(const std::string &key, const std::string &value);
	void				SetBody(const std::string &body);	///< pose aussi Content-Length
	const std::string	&getBody(void) const;
	int					getStatus(void) const;
	const string		&getStatusText(void) const;

	/*===Member Function===*/
	bool				Serialize(std::string &out);
	void				Reset(void);
	static Response		BuildError(int code, const ServerConfig &server);
	void				generateBuiltInError(void);
};

#endif /*RESPONSE_HPP*/
