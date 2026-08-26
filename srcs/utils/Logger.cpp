#include "../../includes/Logger.hpp"
#include <ctime>
#include <iostream>

Logger::Logger(void)
	:_SavedCout(NULL)
	,_SavedCerr(NULL)
	,_Redirected(false)
{

}

Logger::Logger(const string &path)
	:_SavedCout(NULL)
	,_SavedCerr(NULL)
	,_Redirected(false)
{}

Logger::~Logger(void)
{
	if (!this->_Redirected)
		return;
	cout.rdbuf(this->_SavedCout);
	cerr.rdbuf(this->_SavedCerr);
	_File.close();
}

Logger::Logger(const Logger& to_copy)
	:_SavedCout(to_copy._SavedCout)
	,_SavedCerr(to_copy._SavedCerr)
	,_Redirected(to_copy._Redirected)
{}

Logger	&Logger::operator=(const Logger& src)
{
	if (this != &src)
	{
		this->_SavedCout = src._SavedCout;
		this->_SavedCerr = src._SavedCerr;
		this->_Redirected = src._Redirected;
	}
	return (*this);
}

void	Logger::write(const string &level, const string &msg)
{
	time_t	now = time(NULL);
	localtime(&now);

	cout << stamp << " [" << level << "] " << msg << endl;
}