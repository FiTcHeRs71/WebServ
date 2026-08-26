#include "../../includes/Logger.hpp"
#include <ctime>
#include <iostream>

Logger::Logger(void)
{

}

Logger::Logger(const string &path)
	:_SavedCout(NULL)
	,_SavedCerr(NULL)
	,_Redirected(false)
{

}

Logger::~Logger(void)
{
	if (!this->_Redirected)
		return;
	cout.rdbuf(this->_SavedCout);
	cerr.rdbuf(this->_SavedCerr);
	_File.close();
}

Logger::Logger(const Logger& to_copy)
{
	(void)to_copy;
}

Logger	&Logger::operator=(const Logger& src)
{
	(void)src;
	return (*this);
}

void	Logger::write(const string &level, const string &msg)
{
	time_t	now = NULL;
	localtime(&now);

	cout << stamp << " [" << level << "] " << msg << endl
}