#include "../../includes/Autoindex.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <sstream>
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
	if (a.IsDir != b.IsDir)
		return (a.IsDir);
	return (a.Name < b.Name);
}

string	build_autoindex(const string &dir_path, const string &uri)
{
	vector<TDirEntry>	entries;

	if (!list_dir(dir_path, entries))
		return ("");
	(void)uri;
	sort(entries.begin(), entries.end());
	ostringstream oss;
	for(size_t i = 0; i < entries.size(); i++)
		oss << html_escape(entries[i].Name) << (entries[i].IsDir ? "/" : "") << "\n";
	return (oss.str());
}

string	html_escape(const string &s)
{
	string	out;

	for (size_t i = 0; i < s.size(); i++)
	{
		char c = s[i];

		switch (c)
		{
			case '&': out += "&amp;"; break;
			case '<': out += "&lt;"; break;
			case '>': out += "&gt;"; break;
			case '"': out += "&quot;"; break;
			case '\'': out += "&#39;"; break;
			default : out += c;
		}
	}
	return (out);
}

string	url_encode(const string &s)
{
	string	hex = "0123456789ABCDEF";
	string	out;

	for (size_t i = 0; i < s.size(); i++)
	{
		char			c = s[i];
		unsigned char	uc = (unsigned char) c;
		if (isalnum(uc) || c == '-' || c == '_' || c == '.' || c == '~')
			out += c;
		else
		{
			out += '%';
			out += hex[uc >> 4];
			out += hex[uc & 0x0F];
		}
	}
	return (out);
}

