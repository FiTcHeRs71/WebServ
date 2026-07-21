#include "../../includes/CgiProcess.hpp"

	/*===Canonical Form===*/
CgiProcess::CgiProcess(void)
{
	//std::cout << "LocationConfig default constructor called" << std::endl;
}

CgiProcess::~CgiProcess(void)
{
	//std::cout << "LocationConfig default destructor called" << std::endl;
}

CgiProcess::CgiProcess(const CgiProcess& to_copy)
{
	(void)to_copy;
	//std::cout << "LocationConfig copy constructor called" << std::endl;
}
CgiProcess	&CgiProcess::operator=(const CgiProcess& src)
{
	(void)src;
	//std::cout << "LocationConfig operator assignement(=) constructor called" << std::endl;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}

	/*===Getters & Setters===*/
	/**
	 * @brief Retourne le descripteur de lecture du pipe vers le CGI.
	 * @return Le file descriptor en lecture, ou -1 si non disponible.
	 */
int	CgiProcess::GetReadFd(void) const
{
	return (-1);
}

	/**
	 * @brief Retourne le descripteur d'ecriture du pipe vers le CGI.
	 * @return Le file descriptor en ecriture, ou -1 si non disponible.
	 */
int	CgiProcess::GetWriteFd(void) const
{
	return (-1);
}

	/*===Member Function===*/
	/**
 * @brief Demarre l'execution du script CGI correspondant a la requete.
 * @param request  La requete HTTP a transmettre au processus CGI.
 * @param location La configuration de location associee (chemin du CGI, etc.).
 */
void	CgiProcess::Start(const Request &request, const LocationConfig &location)
{
	(void)request;
	(void)location;
}