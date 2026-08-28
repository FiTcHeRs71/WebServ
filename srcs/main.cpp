#include "../includes/Config.hpp"
#include "../includes/ListenSockets.hpp"
#include "../includes/Request.hpp"
#include "../includes/EventLoop.hpp"
#include "../includes/Network.hpp"
#include "../includes/Logger.hpp"

#include <cstddef>
#include <iostream>
#include <exception>
#include <csignal>
#include <sstream>

using namespace std;

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		cout << "USAGE : \n./Webserv <conf_file.conf>" << endl;
		return (1);
	}
	try
	{
		setup_signals();

		ConfigParser	cfg;
		parse(argv[1], cfg);
		ListenSockets servSock(cfg.getAddrPorts());
		Logger	log(DEFAULT_LOG_FILE);
		log.write("info", "webserv started");
		for (size_t i = 0; i < cfg.getAddrPorts().size(); i++)
		{
			ostringstream	oss;

			oss << "listening on " + cfg.getAddrPorts()[i].Host << ":" << cfg.getAddrPorts()[i].Port;
			log.write("info", oss.str());
		}
		EventLoop pollLoop(cfg, servSock);
		pollLoop.Run();
	}
	catch (exception &e)
	{
		cout << "/!\\ Error /!\\ : "<< e.what() << endl;
		return (1);
	}
	return (0);
}


