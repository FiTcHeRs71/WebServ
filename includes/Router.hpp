#ifndef ROUTER_HPP
# define ROUTER_HPP

# include "Response.hpp"
# include "Connection.hpp"
# include "ServerConfig.hpp"
# include "Config.hpp"
# include "Response.hpp"
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>

static map<string, map<string, int> > g_sessions;

struct TMultipartPart
{
	std::string	Name;			///< name="..." du Content-Disposition
	std::string	Filename;		///< filename="...", vide si champ texte simple
	std::string	ContentType;	///< Content-Type: de la part, vide si absent
	std::string	Data;			///< octets bruts, peut contenir des \0
};

bool		parse_multipart(const std::string &body, const std::string &boundary,
						std::vector<TMultipartPart> &out);								///< false si body/boundary/delimiteur invalides
bool		findBoundary(const string &value, string &boundary, size_t idx);			///< lit boundary= ; false si quotes mal formees
string		findParam(const string &headers, const string &key);						///< valeur de name= / filename= / Content-Type:
string		sanitize_filename(const std::string &raw);									///< basename seul, refuse vide, .., leading '.'
Response	Router(const Request &request, const ServerConfig &server,
						Connection &connection);
Response	uploadMultipart(const ServerConfig &server, const LocationConfig &location,
						vector<TMultipartPart> parts);									///< 201 Location du 1er fichier, ou 400/500
Response	upload(const Request &request, const ServerConfig &server,
						const LocationConfig &location);								///< POST raw : body entier = fichier
int			writeInFile(const string &filename, const string &body, string &written);	///< 0 ok, 400, 500 ; written = path reel
int			sanitizeAndWrite(const LocationConfig &location, string &written,
						const string &name, const string &data);						///< -1 nom refuse, 0 ok, >0 HTTP
Response	handleSession(const Request &request, const ServerConfig &server);
#endif