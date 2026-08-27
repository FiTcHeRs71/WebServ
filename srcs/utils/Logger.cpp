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
	this->_SavedCout = cout.rdbuf();
	this->_SavedCerr = cerr.rdbuf();
	cout.rdbuf(this->_File.rdbuf());
	cerr.rdbuf(this->_File.rdbuf());
	this->_Redirected = true;
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
	time_t		now = time(NULL);
	struct tm	*t = localtime(&now);
	char stamp[20];

	strftime(stamp, sizeof(stamp), "%Y/%m/%d %H:%M:%S", t);
	cout << stamp << " [" << level << "] " << msg << endl;
}