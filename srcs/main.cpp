#include "../includes/Config.hpp"
#include "../includes/ListenSockets.hpp"
#include "../includes/Request.hpp"
#include "../includes/EventLoop.hpp"
#include "../includes/Network.hpp"

#include <iostream>
#include <exception>
#include <csignal>

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


