#include "../includes/Config.hpp"
#include "../includes/ListenSockets.hpp"
#include "../includes/Request.hpp"
#include <iostream>
#include <exception>

using namespace std;

/*========================================================================================================*/
/*========================================================================================================*/
/**
 * @brief Pour test ou debug les URI
 */
/*static int	resolve_debug(const ConfigParser &cfg, const string &uri)
{
	if (cfg.getServers().empty())
	{
		cout << "location: NO_SERVER" << endl;
		return (1);
	}

	const ServerConfig		&srv = cfg.getServers()[0];
	const LocationConfig	*loc = srv.Resolve("webserv", 8080, uri);

	if (loc == NULL)
	{
		cout << "location: NULL" << endl;
		return (0);
	}
	cout << "location: " << loc->getPath() << endl;
	cout << "path: " << srv.build_path(*loc, uri) << endl;
	return (0);
}

int main(int argc, char **argv)
{
	ConfigParser	cfg;
	bool			debug = (argc == 4 && string(argv[2]) == "--resolve");

	if (argc != 2 && !debug)
	{
		cout << "USAGE : \n./Webserv <conf_file.conf>" << endl;
		cout << "        ./Webserv <conf_file.conf> --resolve <uri>" << endl;
		return (1);
	}
	try
	{
		parse(argv[1], cfg);
	}
	catch (exception &e)
	{
		cout << "/!\\ Error /!\\ : "<< e.what() << endl;
		return (1);
	}
	if (debug)
		return (resolve_debug(cfg, argv[3]));
	return (0);
}*/
/*========================================================================================================*/
/*main → parse(argv[1], cfg)        // une seule fois, échec = exit 1
     → ListenSockets(cfg.getServers())   // B-01, lit les _Listens
     → boucle poll()              // B-02, tourne jusqu'à SIGINT
        → par requête : selectServer(port) → srv.Resolve(uri) → srv.build_path(...)*/
/*========================================================================================================*/
int main(int argc, char **argv)
{
	if (argc != 2)
	{
		cout << "USAGE : \n./Webserv <conf_file.conf>" << endl;
		return (1);
	}
	try
	{
		ConfigParser	cfg;
		parse(argv[1], cfg);
		ListenSockets servSock(cfg.getServers());
		string str[3]  = {"GE", "T\r\n\\hello.txt?log\r\n\r\nHT", "TP/1.1"};
		Request Req;
		for(size_t i = 0; i < 3; i++){
			Req.Feed(str[i].c_str(), str[i].length());
		}
	}
	catch (exception &e)
	{
		cout << "/!\\ Error /!\\ : "<< e.what() << endl;
		return (1);
	}
	return (0);
}
