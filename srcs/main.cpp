#include "../includes/Config.hpp"
#include "../includes/ListenSockets.hpp"
#include <iostream>
#include <exception>

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
		ConfigParser	cfg;
		parse(argv[1], cfg);
		ListenSockets servSock(cfg.getAddrPorts());
	}
	catch (exception &e)
	{
		cout << "/!\\ Error /!\\ : "<< e.what() << endl;
		return (1);
	}
	return (0);
}
