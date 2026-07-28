#include "../../includes/LocationConfig.hpp"
#include "../../includes/Config.hpp"
#include <set>
#include <stdexcept>

	/*===Canonical Form===*/
LocationConfig::LocationConfig(void)
	:_AutoIndex(false)
	,_HasAutoIndex(false)
	,_ReturnCode(0)
	,_HasReturn(false)
	,_ClientMaxBodySize(0)
	,_HasClientMaxBodySize(false)
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
	,_HasAutoIndex(to_copy._HasAutoIndex)
	,_ReturnCode(to_copy._ReturnCode)
	,_ReturnUrl(to_copy._ReturnUrl)
	,_HasReturn(to_copy._HasReturn)
	,_CgiExt(to_copy._CgiExt)
	,_CgiPass(to_copy._CgiPass)
	,_ClientMaxBodySize(to_copy._ClientMaxBodySize)
	,_HasClientMaxBodySize(to_copy._HasClientMaxBodySize)
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
		this->_HasAutoIndex = src._HasAutoIndex;
		this->_ReturnCode = src._ReturnCode;
		this->_ReturnUrl = src._ReturnUrl;
		this->_HasReturn = src._HasReturn;
		this->_CgiExt = src._CgiExt;
		this->_CgiPass = src._CgiPass;
		this->_ClientMaxBodySize = src._ClientMaxBodySize;
		this->_HasClientMaxBodySize = src._HasClientMaxBodySize;
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
	for (size_t i = 0; i < src._Index.size(); i++)
		flux << src._Index[i] << " | ";
	flux << endl;
	flux << "Auto index = " << src._AutoIndex << endl;
	flux << "Has auto index" << src._HasAutoIndex << endl;
	flux << "Return Code = " << src._ReturnCode << endl;
	flux << "Return URL = " << src._ReturnUrl << endl;
	flux << "Has return = " << src._HasReturn << endl;
	flux << "CGI ext = " << src._CgiExt << endl;
	flux << "CGI pass = " << src._CgiPass << endl;
	flux << "Client max body size = " << src._ClientMaxBodySize << endl;
	flux << "Has client max body size = " << src._HasClientMaxBodySize << endl;
	flux << "=====================" << endl;
	return (flux);
}

void	LocationConfig::parse_location(vector<string>	&token, size_t &i)
{
	size_t flag = token[i].find_first_of("/");
	
	if (flag != 0)
		throw runtime_error(token[i] + " is not a valid PATH");
	this->_Path = token[i];
	i+=2; // saute le PATH + "{"
	while (i < token.size() && token[i] != "}")
	{
		string	key = token[i];
		i++;

		vector<string>	value = collect_values(token, i);
		if (key == "allow_methods")
			this->_Methods = parse_allow_methods(value);
		else if (key == "root")
			this->_Root = parse_root(value);
		else if (key == "index")
			this->_Index = parse_index(value);
		else if (key == "autoindex")
		{
			if (parse_auto_index(value))
				this->_AutoIndex = true;
		}
		else if (key == "cgi_ext")
		{

		}
		else if (key == "cgi_pass")
		{

		}
		else if (key == "client_max_body_size")
		{
			if (value.size() != 1)
				throw runtime_error(key + " needs only one arguments");
			this->_HasClientMaxBodySize = true;
			this->_ClientMaxBodySize = parse_body_size(value[0]);
		}
		else if (key == "return")
		{

		}
		else 
			throw runtime_error(key + " is not a valid instructions in location bloc");
	}
	i++;
}
