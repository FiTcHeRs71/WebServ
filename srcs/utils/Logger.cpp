#include "../../includes/Logger.hpp"
#include <ctime>
#include <iostream>

Logger::Logger(void)
	:_SavedCout(NULL)
	,_SavedCerr(NULL)
	,_Redirected(false)
{}

Logger::Logger(const string &path)
	:_SavedCout(NULL)
	,_SavedCerr(NULL)
	,_Redirected(false)
{
	this->_File.open(path.c_str(), ios::out|ios::app);
	if (!this->_File.is_open())
	{
		cerr << "webserv: [warn] cannot open " << path << ", logging to terminal" << endl;
		return ;
	}
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
	:_SavedCout(to_copy._SavedCout)
	,_SavedCerr(to_copy._SavedCerr)
	,_Redirected(false)
{}

Logger	&Logger::operator=(const Logger& src)
{
	if (this != &src)
	{
		this->_SavedCout = src._SavedCout;
		this->_SavedCerr = src._SavedCerr;
		this->_Redirected = false;
	}
	return (*this);
}

void	Logger::write(const string &level, const string &msg)
{
	time_t	now = time(NULL);
	localtime(&now);

	cout << stamp << " [" << level << "] " << msg << endl;
}