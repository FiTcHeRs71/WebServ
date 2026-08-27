#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <iostream>
# include <fstream>
# include <streambuf>
# include <string>

using namespace std;

class Logger
{
	private:

	ofstream	_File;
	streambuf	*_SavedCout;
	streambuf	*_SavedCerr;
	bool		_Redirected;

	public:

	/*===Canonical Form===*/
	Logger(void);
	Logger(const string &path);
	~Logger(void);
	Logger(const Logger& to_copy);
	Logger&operator=(const Logger& src);

	/*===Member Function===*/
	void	write(const string &level, const string &msg);
};

#endif