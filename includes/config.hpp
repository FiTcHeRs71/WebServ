#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <exception>
#include <iostream>
#include <vector>
#include <set>

using namespace std;

/* === Fonction === */
vector<string>				tokenize(const string &path);
void						check_syntax(const vector<string> &tokens);
const set<string>			&known_directives();

/* === Helpers === */
void			is_valid_file(ifstream &file);

#endif /*CONFIG_HPP*/