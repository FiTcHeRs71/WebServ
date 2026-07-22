# include "../../includes/config.hpp"
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <ostream>
#include <string>

using namespace std;

/**
 * @brief Prend le fichier de configuration et le tokenize.
 * 
 * Il stocke les valeurs dans dans un vector (lexer_config)
 * Il ne fait aucune verification de validite ou synthaxes des arguments
 * Checking au debut de l'accesiblite du file passer en argument
 * @return Le vector remplie de token de non checker du file de configuration
*/
vector<string> tokenize(const string &path)
{
	ifstream		config_file(path.c_str());
	vector<string>	lexer_config;
	string			line;
	string			to_ignore = " \t\n\r\v\f";
	string			specials = ";{}";

	is_valid_file(config_file);

	while (getline(config_file, line))
	{
		size_t	flag_begin = 0;
		size_t	flag_end = 0;

		if (line.size()<= 0 ||line[0] == '#')
			continue;
		while (flag_end < line.size())
		{
			flag_begin = line.find_first_not_of(to_ignore, flag_begin);
			if (flag_begin == string::npos)
				break;
			flag_end = (line.find_first_of(to_ignore + specials, flag_begin));
			if (flag_end != string::npos && specials.find(line[flag_end]) != string::npos)
			{
				if (flag_end > flag_begin)
					lexer_config.push_back(line.substr(flag_begin, flag_end - flag_begin));
				lexer_config.push_back(line.substr(flag_end, 1));
				flag_begin = flag_end + 1;
			}
			else if (flag_end - flag_begin > 0)
			{
				lexer_config.push_back(line.substr(flag_begin, flag_end - flag_begin));
				flag_begin = flag_end;
			}
		}
	}
	return (lexer_config);
}

