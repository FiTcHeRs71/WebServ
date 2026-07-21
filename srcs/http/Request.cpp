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
	int	Request::feed(const char *data, size_t n)
	{
		(void)data;
		(void)n;
		return (0);
	}
