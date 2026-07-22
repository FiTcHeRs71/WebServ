#ifndef CONFIG_HPP
# define CONFIG_HPP

#include <exception>
#include <iostream>
#include <vector>

using namespace std;

/* === Fonction === */
vector<string>	tokenize(const string &path);

/* === Helpers === */
void			is_valid_file(ifstream &file);

#endif /*CONFIG_HPP*/