#include "../../includes/Autoindex.hpp"
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>

static bool	list_dir(const string &dir_path, vector<TDirEntry> &out)
{
	DIR *dir = opendir(dir_path.c_str());
	struct dirent	*entry;
	TDirEntry		Direntry;

	if (dir == NULL)
		return (false); // 403
	while ((entry = readdir(dir)) != NULL)
	{
		string		name = entry->d_name;
		struct stat	stats;
		if(name == "." || name == "..")
			continue;
		string 	full_path = dir_path + name;
		if (stat(full_path.c_str(), &stats))
			continue;
		Direntry.Name = name;
		Direntry.IsDir = S_ISDIR(stats.st_mode);
		Direntry.Size = stats.st_size;
		Direntry.MTime = stats.st_mtime;
		out.push_back(Direntry);
	}
	closedir(dir);
	return (true);
}

bool	operator<(const TDirEntry &a, const TDirEntry &b)
{
	(void)a;
	(void)b;
	return (true);
}

string	build_autoindex(const string &dir_path, const string &uri)
{
	(void)dir_path;
	(void)uri;
	return ("TEST");
}

string	html_escape(const string &s)
{
	(void)s;
	return ("");
}

string	url_encode(const string &s)
{
	(void)s;
	return ("");
}

