#include "../../includes/Autoindex.hpp"
#include <algorithm>
#include <cctype>
#include <clocale>
#include <cstddef>
#include <ctime>
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

/**
 * @brief Formate une date de modification en UTC, style NGINX.
 * @param mtime Champ st_mtime releve par stat().
 * @return "20-Jul-2025 14:19", ou "-" si gmtime echoue.
 */
static string	format_time(time_t mtime)
{
	char		buf[64];
	struct tm	*gmt = gmtime(&mtime);

	if (gmt == NULL)
		return ("-");
	setlocale(LC_TIME, "C");
	strftime(buf, sizeof(buf), "%d-%b-%Y %H:%M", gmt);
	return (buf);
}

/**
 * @brief Rend la taille affichable d'une entree.
 * @param e Entree du listing.
 * @return "-" pour un dossier, la taille en octets sinon.
 */
static string	format_size(const TDirEntry &e)
{
	ostringstream	oss;

	if (e.IsDir)
		return ("-");
	oss << e.Size;
	return (oss.str());
}

/**
 * @brief Genere le listing HTML d'un dossier (autoindex on).
 *
 * Le HTML est autonome : style inline, aucun asset externe. Les noms sont
 * echappes pour l'affichage (html_escape) et encodes pour les liens
 * (url_encode) ; les href sont relatifs a l'URI, avec un '/' final sur les
 * dossiers pour eviter un 301 a chaque clic.
 *
 * @param dir_path Chemin disque du dossier, terminé par '/'.
 * @param uri URI de la requete, terminee par '/'.
 * @return Le HTML complet, ou "" si opendir echoue (le caller rend 403).
 */
string	build_autoindex(const string &dir_path, const string &uri)
{
	vector<TDirEntry>	entries;
	ostringstream		oss;

	if (!list_dir(dir_path, entries))
		return ("");
	sort(entries.begin(), entries.end());

	string	titre = "Index of " + html_escape(uri);
	oss << "<!DOCTYPE html>\n<html>\n<head>\n"
		<< "<meta charset=\"utf-8\">\n"
		<< "<title>" << titre << "</title>\n"
		<< "<style>body{font-family:monospace}"
		<< "td{padding:0 1.5em 0 0;white-space:nowrap}</style>\n"
		<< "</head>\n<body>\n"
		<< "<h1>" << titre << "</h1>\n<hr>\n<table>\n";
	if (uri != "/")
		oss << "<tr><td><a href=\"../\">../</a></td><td></td><td></td></tr>\n";
	for (size_t i = 0; i < entries.size(); i++)
	{
		string	suffix = entries[i].IsDir ? "/" : "";
		string	href = url_encode(entries[i].Name) + suffix;
		string	label = html_escape(entries[i].Name) + suffix;

		oss << "<tr><td><a href=\"" << href << "\">" << label << "</a></td>"
			<< "<td>" << format_size(entries[i]) << "</td>"
			<< "<td>" << format_time(entries[i].MTime) << "</td></tr>\n";
	}
	oss << "</table>\n<hr>\n</body>\n</html>\n";
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

