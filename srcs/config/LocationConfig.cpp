#include "../../includes/LocationConfig.hpp"
#include <set>

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
	:_Path(to_copy._Path)
	,_Methods(to_copy._Methods)
	,_Root(to_copy._Root)
	,_Index(to_copy._Index)
	,_AutoIndex(to_copy._AutoIndex)
	,_HasAutoIndex(to_copy._AutoIndex)
	,_ReturnCode(to_copy._ReturnCode)
	,_ReturnUrl(to_copy._ReturnUrl)
	,_HasReturn(to_copy._HasReturn)
	,_CgiExt(to_copy._CgiExt)
	,_CgiPass(to_copy._CgiPass)
{
	//std::cout << "LocationConfig copy constructor called" << std::endl;
}
LocationConfig	&LocationConfig::operator=(const LocationConfig& src)
{
	//std::cout << "LocationConfig assignement operator(=) constructor called" << std::endl;
	if (this != &src)
	{
		this->_Path = src._Path;
		this->_Methods = src._Methods;
		this->_Root = src._Root;
		this->_Index = src._Index;
		this->_AutoIndex = src._AutoIndex;
		this->_HasAutoIndex = src._AutoIndex;
		this->_ReturnCode = src._ReturnCode;
		this->_ReturnUrl = src._ReturnUrl;
		this->_HasReturn = src._HasReturn;
		this->_CgiExt = src._CgiExt;
		this->_CgiPass = src._CgiPass;
	}
	return (*this);
}

	/*===Getters & Setters===*/

	/*===Member Function===*/
ostream	&operator<<(ostream &flux, const LocationConfig &src)
{
	flux << "=== Location block ===" << endl;
	flux << "Path = " << src._Path << endl;
	flux << "Methods = ";
	set<string>::const_iterator it;
	for (it = src._Methods.begin(); it != src._Methods.end(); ++it)
		flux  << *it << " | ";
	flux << endl;
	flux << "Root = " << src._Root << endl;
	flux << "Index = ";
	for (size_t i = 0; i < src._Root.size(); i++)
		flux << src._Root[i] << " | ";
	flux << endl;
	flux << "Auto index = " << src._AutoIndex << endl;
	flux << "Has auto index" << src._HasAutoIndex << endl;
	flux << "Return Code = " << src._ReturnCode << endl;
	flux << "Return URL = " << src._ReturnUrl << endl;
	flux << "Has return = " << src._HasReturn << endl;
	flux << "CGI ext = " << src._CgiExt << endl;
	flux << "CGI pass = " << src._CgiPass << endl;
	flux << "=====================" << endl;
	return (flux);
}