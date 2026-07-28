#include "../includes/Config.hpp"
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
		parse(argv[1]);
	}
	catch (exception &e)
	{
		cout << "/!\\ Error /!\\ : "<< e.what() << endl;
		return (1);
	}
	return (0);
}
