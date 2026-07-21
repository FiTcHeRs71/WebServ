#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <iostream>
# include <string>

/**
 * @brief Represente une reponse HTTP a construire et a envoyer au client.
 *
 * Regroupe le statut, les en-tetes et le corps de la reponse,
 * et sait se serialiser en une chaine prete a etre ecrite sur le socket.
 */
class Response
{
	private:



	protected:



	public:

	/*===Canonical Form===*/
	Response(void);
	~Response(void);
	Response(const Response& to_copy);
	Response &operator=(const Response& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/
	bool	Serialize(std::string &out);
};

#endif /*RESPONSE_HPP*/