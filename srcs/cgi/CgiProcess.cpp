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
int	CgiProcess::GetReadFd(void) const
{
	return (-1);
}

int	CgiProcess::GetWriteFd(void) const
{
	return (-1);
}

	/*===Member Function===*/