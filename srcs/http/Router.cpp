#include "../../includes/Router.hpp"
#include <fcntl.h>
#include <string>

Response	Rooter(const Request &request,
					const ServerConfig &server,
					const Connection &connection)
{
	Response	res;
	const LocationConfig	*loc = server.Resolve(request.getPath());
	if (!loc)
	{
		// return(res.BuildError(404, serv));
	}
	string	file = server.build_path(*loc, request.getPath());
	if (!file.empty())
	{
		struct stat *statbuf;
		if (stat(file.c_str(), statbuf) < 0)
		{
			// return(res.BuildError(404, serv));
		}
		int fd;
		if ((fd = open(file.c_str(), O_RDONLY)) < 0)
		{
			// return(res.BuildError(403, serv));
		}
		char	buf[4096];
		ssize_t	n;
		string	body;
		while ((n = read(fd, buf, sizeof(buf))) > 0)
			body.append(buf, static_cast<size_t>(n));
		close(fd);
		if (n < 0)
		{
			// return(res.BuildError(403, serv));
		}
		if (n == 0 && body.empty())
		{
			// 200 et content-length = 0
		}

	}
}