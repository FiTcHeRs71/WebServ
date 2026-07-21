#ifndef CGI_PROCESS_HPP
# define CGI_PROCESS_HPP

# include <iostream>

class Request;
class LocationConfig;

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