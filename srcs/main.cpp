#include "../includes/config.hpp"
#include <iostream>
#include <exception>

using namespace std;

int main(int argc, char **argv)
{
	if (argc != 2)
	{
		return (1);
	}
	try
	{
		check_syntax(tokenize(argv[1]));
		cout << "Checking OK " << endl;
	}
	catch (exception &e)
	{
		cout << "/!\\ Error /!\\ : "<< e.what() << endl;
		return (1);
	}
	return (0);
}