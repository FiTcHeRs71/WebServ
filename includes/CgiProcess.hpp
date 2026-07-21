#ifndef CGI_PROCESS_HPP
# define CGI_PROCESS_HPP

# include <iostream>

class Request;
class LocationConfig;

/**
 * @brief Gere le cycle de vie d'un processus CGI externe.
 *
 * Encapsule le lancement du script CGI (fork/exec) ainsi que les
 * descripteurs de fichiers utilises pour communiquer avec lui.
 */
class CgiProcess
{
	private:



	protected:



	public:

	/*===Canonical Form===*/
	CgiProcess(void);
	~CgiProcess(void);
	CgiProcess(const CgiProcess& to_copy);
	CgiProcess &operator=(const CgiProcess& src);

	/*===Getters & Setters===*/
	int		GetReadFd(void) const;
	int		GetWriteFd(void) const;

	/*===Member Function===*/
	void	Start(const Request &request, const LocationConfig &location);
};

#endif /*CGI_PROCESS_HPP*/