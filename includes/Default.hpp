#ifndef DEFAULT_HPP
# define DEFAULT_HPP

# include <iostream>

using namespace std;

/**
 * @brief Valeurs appliquees quand une directive est absente du .conf
 *
 * Ces defauts sont poses par ConfigParser::apply_defaults(), sauf DEFAULT_PORT
 * qui sert aussi a parse_listen_directive() quand un listen ne donne qu'une
 * adresse. Elles reprennent le comportement de NGINX.
 */
const size_t		DEFAULT_BODY_SIZE	= 1048576;		///< 1 Mo, comme la directive client_max_body_size de NGINX
const char *const	DEFAULT_ROOT		= "./www";		///< Racine disque d'une location sans root
const char *const	DEFAULT_INDEX		= "index.html";	///< Fichier servi quand l'URI designe un repertoire
const char *const	DEFAULT_HOST		= "0.0.0.0";		///< Toutes les interfaces, valeur d'un listen implicite
const int			DEFAULT_PORT		= 80;			///< Port d'un listen sans port explicite
const char *const	LOC_NO_SUPPORTED	= "=,~^@";
#endif