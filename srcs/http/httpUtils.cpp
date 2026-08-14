#include "../../includes/Request.hpp"


/*===Fonctions externes===*/
/**
 * @brief Fonction permettant de trim les espaes de debut det de fin
 *
 * @param s
 */
void trim(string& s){
	size_t end = s.find_last_not_of(" \t\r\n");
	if (end == string::npos)
		return ;
	s.erase(end + 1);
	size_t first = s.find_first_not_of(" \t");
	if (first == string::npos)
		return;
	s.erase(0, first);
}

/**
 * @brief Permet de mettre une string en lowercase
 *
 * @param key
 */
void MyToLower(string& key){
	for(size_t i = 0; i < key.size(); i++){
		key[i] = tolower(key[i]);
	}
}

/**
 * @brief check si les 2 char sont en hexadecimal
 *
 * @param c char 1
 * @param d char 2
 * @return true
 * @return false
 */
bool isHexa(char c, char d){
	if (((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f')) &&
		((d >= '0' && d <= '9') || (d >= 'A' && d <= 'F') || (d >= 'a' && d <= 'f')))
		return true;
	return false;

}

/**
 * @brief Permet de convertir la valeur hexadecimal de l'url encoding
 *
 * @param c char 1
 * @param d char 2
 * @return char converti
 */
char convertToHexa(char c,char d){
	int convertC = 0;
	int convertD = 0;
	if (c >='0' && c <= '9')
		convertC = c - '0';
	else if (c >= 'a' && c <= 'f')
		convertC = c - 'a' + 10;
	else if (c >= 'A' && c <= 'F')
		convertC = c - 'A' + 10;
	if (d >='0' && d <= '9')
		convertD = d - '0';
	else if (d >= 'a' && d <= 'f')
		convertD = d - 'a' + 10;
	else if (d >= 'A' && d <= 'F')
		convertD = d - 'A' + 10;
	char val = (convertC * 16) + convertD;
	return val;
}
