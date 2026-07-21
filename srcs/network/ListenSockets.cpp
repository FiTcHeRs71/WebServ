# include "../../includes/ListenSockets.hpp"

	/*===Canonical Form===*/
ListenSockets::ListenSockets(const std::vector<ServerConfig> &servers)
{
	(void)servers;
	//std::cout << "ListenSockets default constructor called" << std::endl;
}

ListenSockets::~ListenSockets(void)
{
	//std::cout << "ListenSockets default destructor called" << std::endl;
}

ListenSockets::ListenSockets(const ListenSockets& to_copy)
{
	(void)to_copy;
	//std::cout << "ListenSockets copy constructor called" << std::endl;
}

ListenSockets	&ListenSockets::operator=(const ListenSockets& src)
{
	(void)src;
	//std::cout << "ListenSockets assignement operator(=) constructor called" << std::endl;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}


	/*===Getters & Setters===*/


	/*===Member Function===*/