#include "../../includes/Router.hpp"
#include <string>
#include <fstream>
#include <sstream>
#include <cctype>
#include <fcntl.h>

static map<string, map<string, int> > g_sessions;

/**
 * @brief true si key ne commence pas au milieu d'un identifiant.
 *
 * Evite que find("name=") matche l'interieur de "filename=".
 * idx == 0, ou le caractere precedent n'est pas alphanumerique.
 *
 * @param s Bloc d'en-tetes de la part.
 * @param idx Index du debut de la cle candidate.
 * @return false -> findParam passe au match suivant.
 */
static bool	isStartParam(const string &s, size_t idx)
{
	if (idx == 0)
		return true;
	unsigned char c = static_cast<unsigned char>(s[idx - 1]);
	return (!isalnum(c));
}

/**
 * @brief Extrait la valeur d'un parametre ou d'un header dans une part multipart.
 *
 * Guillemets optionnels. Un match interne a un autre token est ignore
 * (filename= vs name=) via isStartParam. Quote ouvrante sans fermante -> "".
 *
 * @param headers En-tetes de la part (ou le Content-Type de la requete).
 * @param key Cle avec son separateur : "name=", "filename=", "Content-Type: ".
 * @return Valeur sans guillemets, ou "" si absent / mal forme.
 */
string	findParam(const string &headers, const string &key)
{
	size_t idx = 0;

	while ((idx = headers.find(key, idx)) != string::npos)
	{
		if (!isStartParam(headers, idx))
		{
			idx += key.size();
			continue ;
		}
		idx += key.size();
		if (idx < headers.size() && headers[idx] == '"')
		{
			size_t endQuote = headers.find('"', idx + 1);
			if (endQuote == string::npos)
				return "";
			return (headers.substr(idx + 1, endQuote - idx - 1));
		}
		size_t end = idx;
		while (end < headers.size() && headers[end] != ' '
				&& headers[end] != '\t' && headers[end] != '\r'
				&& headers[end] != ';')
				end++;
		return (headers.substr(idx, end - idx));
	}
	return "";
}

/**
 * @brief Ecrit body en binaire dans filename. Collision : suffixe _1, _2 avant l'extension.
 *
 * Ne cree pas le dossier parent : ofstream fail -> 500. Une entree existante
 * qui n'est pas un fichier regulier -> 400. Le '.' d'extension est cherche
 * apres le dernier '/' (pour ne pas matcher le '.' de ./www).
 * Pas d'extension -> suffixe en fin de nom (Makefile -> Makefile_1).
 *
 * @param filename Chemin cible (upload_store + '/' + basename).
 * @param body Octets bruts, peut contenir des NUL.
 * @param written Out : chemin reellement ecrit (avec suffixe si collision).
 * @return 0 ok, 400 collision avec un non-fichier, 500 open/write fail.
 */
int	writeInFile(const string &filename, const string &body, string &written)
{
	string path = filename;
	struct stat st;
	for (size_t i = 1; stat(path.c_str(), &st) == 0; i++)
	{
		if (S_ISREG(st.st_mode))
		{
			path = filename;
			size_t suffix = path.rfind('.');
			size_t slash = path.rfind('/');
			if (suffix == string::npos || slash > suffix)
				suffix = path.size();
			ostringstream oss;
			oss << "_" << i;
			path.insert(suffix, oss.str());
			continue ;
		}
		else
			return (400);
	}
	ofstream	file(path.c_str(), ios::binary);
	if (!file)
		return (500);
	if (body.size() > 0)
		file.write(body.c_str(), body.size());
	written = path;
	return (0);
}

/**
 * @brief sanitize_filename(name) puis writeInFile dans location.getUploadStore().
 *
 * @param location Location qui porte upload_store.
 * @param written Out : path disque ecrit (si return 0).
 * @param name Filename client (part) ou URI (POST raw).
 * @param data Octets a ecrire.
 * @return -1 nom refuse (caller skip / 400), 0 ok, >0 code HTTP (400 ou 500).
 */
int	sanitizeAndWrite(const LocationConfig &location, string &written, const string &name, const string &data)
{
	string basename = sanitize_filename(name);
	if (basename.empty())
		return -1;
	string path = location.getUploadStore() + "/" + basename;
	int code = writeInFile(path, data, written);
	return (code);
}

/**
 * @brief Ecrit toutes les parts qui ont un filename ; 201 + Location du premier fichier.
 *
 * Parts sans filename (champs texte d'un formulaire) ignorees, pas de fichier vide.
 * Aucun fichier ecrit -> 400. Location est une URI (getPath() + basename reel,
 * collision comprise), jamais un chemin disque.
 *
 * @param server Pour BuildError.
 * @param location upload_store + getPath() pour le header Location.
 * @param parts Sortie de parse_multipart.
 * @return 201, 400 ou 500.
 */
Response uploadMultipart(const ServerConfig &server,
						const LocationConfig &location,
						vector<TMultipartPart> parts)
{
	string locName;
	for (size_t i = 0; i < parts.size(); i++)
	{
		string written;
		int code = sanitizeAndWrite(location, written, parts[i].Filename, parts[i].Data);
		if (code < 0)
			continue ;
		else if (code > 0)
			return (Response::BuildError(code, server));
		else if (locName.empty())
		{
			size_t slash = written.rfind('/');
			locName = (slash == string::npos) ? written : written.substr(slash + 1);
		}
	}
	if (locName.empty())
		return (Response::BuildError(400, server));
	Response res;
	res.SetStatus(201);
	res.SetBody("");
	res.SetHeader("Location", location.getPath() + "/" + locName);
	return (res);
}

/**
 * @brief POST brut : tout le body est le fichier, nom = dernier segment de l'URI.
 *
 * @param request getBody() + getPath() (ex. /upload/photo.png -> photo.png).
 * @param server Pour BuildError.
 * @param location upload_store.
 * @return 201 + Location, ou 400/500.
 */
Response	upload(const Request &request,
						const ServerConfig &server,
						const LocationConfig &location)
{
	string body = request.getBody();
	string written;
	int code = sanitizeAndWrite(location, written, request.getPath(), body);
	if (code < 0)
		return (Response::BuildError(400, server));
	else if (code > 0)
		return (Response::BuildError(code, server));
	size_t slash = written.rfind('/');
	string locName = (slash == string::npos) ? written : written.substr(slash + 1);
	Response res;
	res.SetStatus(201);
	res.SetBody("");
	res.SetHeader("Location", location.getPath() + "/" + locName);
	return (res);
}

/**
 * @brief Lit la valeur de boundary= a partir de idx (juste apres le '=').
 *
 * Guillemets optionnels. Espace, tab ou ';' hors quotes terminent la valeur.
 * Quote ouvrante sans fermante -> false. Le '"' d'ouverture n'est pas copie
 * dans boundary.
 *
 * @param value Header Content-Type complet.
 * @param boundary Out : token sans quotes.
 * @param idx Index du premier caractere de la valeur (idx += 9 apres "boundary=").
 * @return false si quotes mal formees.
 */
bool	findBoundary( const string &value, string &boundary, size_t idx)
{
	bool quote = false;
	for (size_t i = idx; i < value.size(); i++)
	{
		if (value[i] == '\"')
		{
			if (i == idx)
			{
				quote = true;
				continue ;
			}
			else if (i > idx && quote == true)
			{
				quote = false;
				break ;
			}
			else
				return false;
		}
		else if ((value[i] == ' ' || value[i] == '	' || value[i] == ';') && !quote)
			break ;
		boundary += value[i];
	}
	if (quote == true)
			return false;
	return true;
}

string	randSessionId(void)
{
	int	fd = open("/dev/urandom", O_RDONLY);
	char	buf[16];
	string	hex = "0123456789abcdef";
	string	id;

	if (fd < 0 || read(fd, buf, 16) != 16)
	{
		if (fd >= 0)
			close(fd);
		return "";
	}
	close(fd);
	for (int i = 0; i < 16; i++)
	{
		unsigned char c = static_cast<unsigned char>(buf[i]);
		id += hex[c >> 4];
		id += hex[c & 15];
	}
	return id;
}

Response	handleSession(const Request &request, const ServerConfig &server)
{
	Response	res;
	string		sid = request.getCookie("sessionid");
	int		n;

	if (sid.empty() || g_sessions.find(sid) == g_sessions.end())
	{
		sid = randSessionId();
		if (sid.empty())
			return (Response::BuildError(500, server));
		g_sessions[sid]["visits"] = 0;
		res.AddSetCookie("sessionid=" + sid + "; Path=/; HttpOnly");
	}
	g_sessions[sid]["visits"]++;
	n = g_sessions[sid]["visits"];
	ostringstream oss;

	oss << "<!DOCTYPE html>\n<html><body>"
		<< "<h1>session</h1>"
		<< "<p>id: " << sid << "</p>"
		<< "<p>visits: " << n << "</p>"
		<< "</body></html>\n";
	res.SetStatus(200);
	res.SetHeader("Content-Type", "text/html; charset=utf-8");
	res.SetBody(oss.str());
	return res;
}