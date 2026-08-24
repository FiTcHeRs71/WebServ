#ifndef DEFAULT_HPP
# define DEFAULT_HPP

#include <ctime>
# include <iostream>

using namespace std;

# define MAXHEADERS		100		 ///< 100 Headers maximum par requetes
# define REQUESTMAXSIZE 8000	///< 8000 octets maximum par requetes
# define BUFFER_SIZE	8192
/**
 * @brief Valeurs appliquees quand une directive est absente du .conf
 *
 * Ces defauts sont poses par ConfigParser::apply_defaults(), sauf DEFAULT_PORT
 * qui sert aussi a parse_listen_directive() quand un listen ne donne qu'une
 * adresse. Elles reprennent le comportement de NGINX.
 */
const char *const	DEFAULT_ROOT		= "./www";		///< Racine disque d'une location sans root
const char *const	DEFAULT_INDEX		= "index.html";	///< Fichier servi quand l'URI designe un repertoire
const char *const	LOC_NO_SUPPORTED	= "=~^@";		///< Arguments de location non supporterpar notre webserv
const char *const	DEFAULT_HOST		= "0.0.0.0";	///< Toutes les interfaces, valeur d'un listen implicite
const size_t		DEFAULT_BODY_SIZE	= 1048576;		///< 1 Mo, comme la directive client_max_body_size de NGINX
const int			DEFAULT_PORT		= 80;			///< Port d'un listen sans port explicite
const time_t		TIMEOUT_HEADER		= 30;			///< requete incomplete -> 408
const time_t		TIMEOUT_IDLE		= 60;			///< keep-alive inactif -> close silencieux

#endif
