#include "../includes/Config.hpp"
#include "../includes/ListenSockets.hpp"
#include "../includes/Request.hpp"
#include "../includes/EventLoop.hpp"
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
		string str[11]  = {"POST /upl", "o%3cad//Fi", "le///admin/log""?type=image H", "TTP/1.1\r\n", "HOSt: loca", "lhost:8080\r\n",
						"Content-Type:", " text/plain\r\n", "Content-Length: 11\r\n\r\nHell", "o world"};
		Request Req;
		for(size_t i = 0; i < 9; i++){
			Req.Feed(str[i].c_str(), str[i].length());
		}
		cout << Req;
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


