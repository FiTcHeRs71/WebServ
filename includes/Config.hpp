#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <iostream>
#include <vector>
#include <set>

using namespace std;

/**
 * @brief Reprensete toute la pahse de parsing du .conf
 *
 * _LexerConfig contient sous forme de token non checker le .conf
 */
class ConfigParser
{
	private:

	vector<string>	_LexerConfig;

	public:

	/*===Canonical Form===*/
	ConfigParser(void);
	~ConfigParser(void);
	ConfigParser(const ConfigParser& to_copy);
	ConfigParser&operator=(const ConfigParser& src);

	/*===Getters & Setters===*/
	

	/*===Member Function===*/
	void			check_syntax(const vector<string> &tokens);
	void			tokenize(const string &path);
	friend void		parse(const string &argv1);
};

/* === Helpers === */
void				is_valid_file(ifstream &file);
void				parse(const string &argv1);
const set<string>	&known_directives();

#endif /*CONFIG_HPP*/