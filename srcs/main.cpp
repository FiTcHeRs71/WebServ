#include "../includes/Config.hpp"
#include <iostream>
#include <exception>

using namespace std;

/**
 * @brief Mode debug : resout une URI et affiche la location + le chemin disque.
 *
 * Sert aux tests unitaires du module config (tests/test_resolve.sh), la sortie
 * tient en deux lignes "location: ..." et "path: ..." pour etre parsable en shell.
 * @param cfg Le parser deja rempli.
 * @param uri L'URI a resoudre.
 * @return 0 si le premier bloc server existe, 1 sinon.
 */
static int	resolve_debug(const ConfigParser &cfg, const string &uri)
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
}
