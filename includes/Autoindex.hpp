#ifndef AUTOINDEX_HPP
# define AUTOINDEX_HPP

# include <ctime>
# include <iostream>
# include <string>
# include <sys/types.h>

using namespace std;

struct	TDirEntry
{
	string	Name;
	bool	IsDir;
	off_t	Size;
	time_t	MTime;
};

bool	operator<(const TDirEntry &a, const TDirEntry &b);
string	build_autoindex(const string &dir_path, const string &uri);
string	html_escape(const string &s);
string	url_encode(const string &s);

#endif