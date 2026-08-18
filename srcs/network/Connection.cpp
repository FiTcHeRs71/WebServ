#include "../../includes/Connection.hpp"

	/*===Canonical Form===*/
Connection::Connection(void)
{
	//std::cout << "Connection default constructor called" << std::endl;
}

Connection::~Connection(void)
{
	//std::cout << "Connection default destructor called" << std::endl;
}

Connection::Connection(const Connection& to_copy)
{
	(void)to_copy;
	//std::cout << "Connection copy constructor called" << std::endl;
}

Connection	&Connection::operator=(const Connection& src)
{
	(void)src;
	//std::cout << "Connection operator assignement (=) constructor called" << std::endl;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}

	/*===Getters & Setters===*/
int	Connection::getFd(void) const
{
	return(this->_Fd);
}

size_t		Connection::getGroupIndex(void) const
{
	return(_GroupIndex);
}

std::string	Connection::getIpv4(void) const
{
	return(_IpV4);
}

void		Connection::setIpV4(std::string src)
{
	this->_IpV4 = src;
}

	/*===Member Function===*/