#ifndef ROOTER_HPP
# define ROOTER_HPP

# include "Response.hpp"
# include "Connection.hpp"
# include "ServerConfig.hpp"
# include "Config.hpp"
# include "Response.hpp"
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>

struct TMultipartPart
{
	std::string	Name;			///< name="..." du Content-Disposition
	std::string	Filename;		///< filename="...", vide si champ texte simple
	std::string	ContentType;
	std::string	Data;			///< octets bruts, peut contenir des \0
};

bool		parse_multipart(const std::string &body, const std::string &boundary,
						std::vector<TMultipartPart> &out);
std::string	sanitize_filename(const std::string &raw);	///< basename seul, refuse .. /
Response	Router(const Request &request,
				const ServerConfig &server,
				Connection &connection);

#endif