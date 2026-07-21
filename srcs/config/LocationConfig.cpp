#include "../../includes/LocationConfig.hpp"

	/*===Canonical Form===*/
LocationConfig::LocationConfig(void)
{
	//std::cout << "LocationConfig default constructor called" << std::endl;
}

LocationConfig::~LocationConfig(void)
{
	//std::cout << "LocationConfig default destructor called" << std::endl;
}

LocationConfig::LocationConfig(const LocationConfig& to_copy)
{
	(void)to_copy;
	//std::cout << "LocationConfig copy constructor called" << std::endl;
}
LocationConfig	&LocationConfig::operator=(const LocationConfig& src)
{
	(void)src;
	//std::cout << "LocationConfig assignement operator(=) constructor called" << std::endl;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}

	/*===Getters & Setters===*/

	/*===Member Function===*/
