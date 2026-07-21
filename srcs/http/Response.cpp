#include "../../includes/Response.hpp"

	/*===Canonical Form===*/
Response::Response(void)
{
	//std::cout << "Response default constructor called" << std::endl;
}

Response::~Response(void)
{
	//std::cout << "Response default destructor called" << std::endl;
}

Response::Response(const Response& to_copy)
{
	(void)to_copy;
	//std::cout << "Response copy constructor called" << std::endl;
}

Response	&Response::operator=(const Response& src)
{
	(void)src;
	//std::cout << "Response operator assignement(=) constructor called" << std::endl;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}

	/*===Getters & Setters===*/


	/*===Member Function===*/
bool	Response::Serialize(std::string &out)
{
	(void)out;
	return (true);
}