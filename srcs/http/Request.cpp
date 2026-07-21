#include "../../includes/Request.hpp"

	/*===Canonical Form===*/
Request::Request(void)
{
	//std::cout << "Request default constructor called" << std::endl;
}

Request::~Request(void)
{
	//std::cout << "Request default destructor called" << std::endl;
}

Request::Request(const Request& to_copy)
{
	(void)to_copy;
	//std::cout << "Request copy constructor called" << std::endl;
}

Request	&Request::operator=(const Request& src)
{
	(void)src;
	//std::cout << "Request operator assignement (=) constructor called" << std::endl;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}

	/*===Getters & Setters===*/


	/*===Member Function===*/

	/**
	 * @brief Alimente le parseur de requete avec un nouveau bloc de donnees.
	 * @param data Pointeur vers les octets recus (non necessairement termines par un caractere nul).
	 * @param n    Nombre d'octets valides dans data.
	 * @return Le statut du parsing (a definir : ex. -1 erreur, 0 incomplet, 1 termine).
	 */
	int	Request::Feed(const char *data, size_t n)
	{
		(void)data;
		(void)n;
		return (0);
	}
