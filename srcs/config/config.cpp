# include "../../includes/config.hpp"
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <ostream>
#include <stdexcept>
#include <string>
#include <vector>
#include <set>


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
		size_t	first = line.find_first_not_of(to_ignore);

		if (first == string::npos || line[first] == '#')
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


void	check_syntax(const vector<string> &tokens)
{
	vector<string>	block_stack;
	size_t			i = 0;

	while (i < tokens.size())
	{
		const string &tok = tokens[i];

		if (tok == "}")
		{
			if (block_stack.empty())
				throw invalid_argument("Closing brace expected");
			block_stack.pop_back();
			i++;
			continue;
		}
		else if (!known_directives().count(tok))
			throw runtime_error("directive inconnue : " + tok);
		if (tok == "server" && !block_stack.empty())
			throw runtime_error("Cannot have a server block inside a otherone");
		if (tok == "location" && (block_stack.empty() || block_stack.back() != "server"))
			throw runtime_error("Location block must be inside a server block");
		if (tok != "server" && tok != "location" && block_stack.empty())
			throw runtime_error(tok + "is out side on any blocks");

		i++;

		if (tok == "server" || tok == "location")
		{
			if (tok == "location")
			{
				if (i >= tokens.size() || tokens[i] == "{")
					throw runtime_error("Location without PATH");
				i++;
			}
			if (i >= tokens.size() || tokens[i] != "{")
				throw runtime_error("'{' expected after " + tok);
			block_stack.push_back(tok);
			i++;
			continue;
		}

		size_t	value_count = 0;
		while (i < tokens.size() && tokens[i] != ";" && tokens[i] != "}" && known_directives().count(tok))
		{
			value_count++;
			i++;
		}
		if (value_count == 0)
			throw runtime_error("directive '" + tok + "' without value");
		if (i >= tokens.size() || tokens[i] != ";")
			throw runtime_error("';' missing after '" + tok + "'");
		i++;
	}
	if (!block_stack.empty())
		throw runtime_error("Brace not closed");
}
