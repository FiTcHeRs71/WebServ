#include "../../includes/Config.hpp"
#include <algorithm>
#include <cctype>
#include <cerrno>
#include <climits>
#include <cstddef>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>
#include <sstream>

using namespace std;

/**
 * @brief Check si le fichier de configuration est ouvrable / lisible et non vide.
 *
 * @param file Flux ouvert sur le fichier passe en argument du programme.
 * @return void, throw si le fichier est inaccessible ou vide.
*/
void	is_valid_file(ifstream &file)
{
	if (!file.is_open())
		throw invalid_argument("Cannot open configuration file");
	if (file.peek() == ifstream::traits_type::eof())
		throw invalid_argument("configuration file is empty");
}

/**
 * @brief Contient toutes les directives (key) valides acceptees dans le .conf
 * ex : ->listen<-	0.0.0.0:8080;
 *
 * @return le set des directives connues.
*/
const set<string> &known_directives(void)
{
	static const string names[] = {
		"listen", "server_name", "client_max_body_size", "error_page",
		"allow_methods", "root", "index", "autoindex",
		"cgi_ext", "cgi_pass", "return", "location", "server",
		"upload_store"
	};
	static const set<string> s(names, names + sizeof(names) / sizeof(names[0]));
	return (s);
}

/**
 * @brief Contient toutes les methodes HTTP acceptees par notre webserv
 *
 * Actuel GET/POST/DELETE
 * @return le set des methodes connues.
*/
const set<string> &known_methods(void)
{
	static const string names[] = {
		"GET", "POST", "DELETE"
	};
	static const set<string> s(names, names + sizeof(names) / sizeof(names[0]));
	return (s);
}

/**
 * @brief Contient toutes les parametres de listen existant dans NGINX
 *
 * @return le set des parametres de listen connues
*/
const set<string>	&known_listen_parameters(void)
{
	static const string names[] = {
		"ssl", "http2", "quic", "proxy_protocol", "deferred", "bind",
		"reuseport", "multipath", "backlog", "rcvbuf", "sndbuf", "setfib",
		"fastopen", "accept_filter", "ipv6only", "so_keepalive"
	};
	static const set<string> s(names, names + sizeof(names) / sizeof(names[0]));
	return (s);
}

/**
 * @brief Check si une chaine est composee uniquement de chiffres.
 *
 * Sert a split_addr_port() pour appliquer la regle NGINX : dans une directive
 * listen, un token entierement numerique est un port, sinon une adresse.
 * @param s Le token a tester.
 * @return true si s est non vide et ne contient que des chiffres, false sinon.
*/
bool	is_all_digits(const string &s)
{
	if (s.empty())
		return (false);
	for (std::string::size_type i = 0; i < s.size(); ++i)
	{
		if (!std::isdigit(static_cast<unsigned char>(s[i])))
			return (false);
	}
	return (true);
}

/**
 * @brief Verifie qu'une chaine est une adresse IPv4 bien formee.
 *
 * Attend 4 segments numeriques separes par 3 points, chacun <= 255,
 * sans zero non significatif en tete (010 est refuse).
 * @param host La partie host du token listen (avant le ":").
 * @return true si l'IPv4 est valide, false sinon.
*/
bool	is_valid_ipv4(const string &host)
{
	size_t	start = 0;
	size_t	i = 0;
	size_t	dot;

	if (count(host.begin(), host.end(), '.') != 3)
		return (false);
	while (i < 4)
	{
		dot = host.find('.', start);
		string	segment = host.substr(start, dot - start);

		if (segment.empty() || segment.size() > 3)
			return (false);
		if (segment.find_first_not_of("0123456789") != string::npos)
			return (false);
		if (segment.size() > 1 && segment[0] == '0')
			return (false);
		if (atoi(segment.c_str()) > 255)
			return (false);
		start = dot + 1;
		i++;
	}
	return (true);
}

/**
 * @brief Fait la conversion de la value donnee par client_max_body_size en size_t
 * Check si un suffixe de taille est present en fin de value (KkMm).
 * Check si la value n'est pas empty, negative ou trop grande (> INT_MAX).
 *
 * @param value La valeur associee a la key <client_max_body_size> (ex : "10M").
 * @return la taille en octets convertie
*/
size_t	parse_body_size(const string &value)
{
	int		multiplier;
	long	size_converted;
	char*	p_end = NULL;
	errno = 0;

	if (value.empty())
		throw runtime_error(value + " is not a valid body size");
	size_converted = strtol(value.c_str(), &p_end, 10);
	if (p_end == value.c_str() || size_converted < 0 || errno == ERANGE)
		throw runtime_error(value + " is not a valid body size");
	if (*p_end == '\0')
		multiplier = 1;
	else if ((*p_end == 'K' || *p_end == 'k') && p_end[1] == '\0')
		multiplier = 1024;
	else if ((*p_end == 'M' || *p_end == 'm') && p_end[1] == '\0')
		multiplier = 1024 * 1024;
	else
		throw runtime_error(value + " is not a valid body size");
	if (size_converted > INT_MAX / multiplier)
		throw runtime_error(value + " is a too big body size");
	return (static_cast<size_t>(size_converted * multiplier));
}

/**
 * @brief Cree une map STL contenant le code d'erreur associe au PATH de la page dediee
 * Check si le code d'erreur depasse 505 ou si la value est negative ou contient des caracteres
 * non numeriques. Le dernier element de value est le PATH, tous les precedents sont des codes.
 *
 * @param value Tous les arguments contenus entre la key <error_page> et le prochain ";"
 * @param j Index de lecture dans value, avance jusqu'au dernier code traite.
 * @return une map[<Error_code>] = "PATH"
*/
map<int, string>	parse_error_pages(const vector<string> &value, size_t &j)
{
	map<int, string>	map;
	long	size_converted;
	char*	p_end = NULL;
	
	if (value.size() < 2)
		throw runtime_error("Error pages missing a elements, minimum correct value needed is 2 or more");
	while (j < value.size() - 1)
	{
		errno = 0;
		size_converted = strtol(value[j].c_str(), &p_end, 10);
		if (size_converted > 505 || size_converted < 0 || errno == ERANGE || *p_end != '\0')
			throw runtime_error( value[j] + " is not a valid code page error");
		map[static_cast<int>(size_converted)] = value.back();
		j++;
	}
	return (map);
}

/**
 * @brief Remplit un vecteur de toutes les valeurs associees a une key
 *
 * Remplit un vecteur de tous les elements contenus entre la key et ";"
 *
 * @param token La liste complete des tokens issue de tokenize().
 * @param i Index positionne sur la premiere valeur, avance apres le ";" consomme.
 * @return vecteur rempli des valeurs
*/
vector<string>		collect_values(vector<string> &token, size_t &i)
{
	vector<string>	values;

	while (token[i] != ";")
	{
		values.push_back(token[i]);
		i++;
	}
	i++; // saute le ";"
	return (values);
}

/**
 * @brief Prend tous les arguments de <allow_methods> et verifie leurs valeurs.
 *
 * @param value contient tous les arguments contenue entre le key <allow_methods> et le prochain ";"
 * @return le set des methodes autorisees pour cette location
*/
set<string>	parse_allow_methods(const vector<string> &value)
{
	set<string>	set;

	for (size_t i = 0; i < value.size(); i++)
	{
		if (!known_methods().count(value[i]))
			throw runtime_error("Unknow directives : " + value[i]);
		set.insert(value[i]);
	}
	return (set);
}

/**
 * @brief Prend tous les arguments de <root> et verifie leurs valeurs.
 *
 * @param value contient tous les arguments contenue entre le key <root> et le prochain ";"
 * @return le chemin racine, un seul argument est accepte
*/
string	parse_root(const vector<string> &value)
{
	if (value.size() > 1)
		throw runtime_error("Too much arguments for key <ROOT>");
	return (value[0]);
}

/**
 * @brief Prend tous les arguments de <index> et verifie leurs valeurs.
 *
 * @param value contient tous les arguments contenue entre le key <index> et le prochain ";"
 * @return le vecteur des fichiers index, dans l'ordre de priorite
*/
vector<string>	parse_index(const vector<string> &value)
{
	if (value.size() == 0)
		throw runtime_error("Key index in location bloc need at leat one value");
	for (size_t i = 0; i < value.size(); i++)
	{
		if (value[i].empty())
			throw runtime_error ("Key index in location bloc has a empty arguments");
	}
	// TODO checker si index.html souvre ?
	return (value);
}

/**
 * @brief Prend tous les arguments de <auto_index> et verifie leurs valeurs.
 *
 * @param value contient tous les arguments contenue entre le key <auto_index> et le prochain ";"
 * @return true si "on", false si "off"
*/
bool	parse_auto_index(const vector<string> &value)
{
	if (value.size() != 1)
		throw runtime_error("autoindex needs only one arguments");
	if (value[0] != "on" && value[0] != "off")
		throw runtime_error(value[0] + "is not a valid argument for key autoindex");
	if (value[0] == "on")
		return (true);
	return (false);
}

/**
 * @brief Prend tous les arguments de <cgi_ext> et verifie leurs valeurs.
 *
 * @param value contient tous les arguments contenue entre le key <cgi_ext> et le prochain ";"
 * @return l'extension CGI validee, elle doit commencer par un "." (ex : ".py")
*/
string	parse_cgi_ext(const vector<string> &value)
{
	if (value.size() != 1)
		throw runtime_error("cgi_ext takes exactly one extension");
	if (value[0].size() < 2 || value[0][0] != '.')
		throw runtime_error(value[0] + " is not a valid CGI extensions");
	return (value[0]);
}

/**
 * @brief Prend tous les arguments de <cgi_pass> et verifie leurs valeurs.
 *
 * @param value contient tous les arguments contenue entre le key <cgi_pass> et le prochain ";"
 * @return le chemin de l'interpreteur CGI, un seul argument est accepte
*/
string	parse_cgi_pass(const vector<string> &value)
{
	if (value.size() != 1)
		throw runtime_error("cgi_pass takes exactly one extension");
	//check si le path du cgi est bon ?
	return (value[0]);
}

/**
 * @brief Verifie le code HTTP de la directive <return>.
 *
 * Un code de redirection (3xx) exige une <URL> cible, donc deux arguments.
 * @param value Le premier argument de <return>, cense etre le code HTTP.
 * @param nb_args Le nombre total d'arguments passes a <return>.
 * @return le code HTTP converti, compris entre 100 et 599
*/
int	parser_return_code(const string &value, const size_t nb_args)
{
	for (size_t i = 0; i < value.size(); i++)
	{
		if(!isdigit(static_cast<unsigned char>(value[i])))
			throw runtime_error(value + " is not a valid return code");
	}
	char	*end;
	errno = 0;
	long	code = strtol(value.c_str(), &end, 10);
	if (code > 299 && code < 400 && nb_args == 1)
		throw runtime_error (value + " needs to have target <URL>");
	if (code < 100 || code > 599 || errno == ERANGE || *end != '\0')
		throw runtime_error(value + "is not a valid HTTP status code");
	return (static_cast<int>(code));
}

/**
 * @brief Prend tous les arguments de <upload_store> et verifie leurs valeurs.
 *
 * @param value contient tous les arguments contenue entre le key <upload_store> et le prochain ";"
 * @return le chemin de stockage des uploads, un seul argument est accepte
*/
string	parse_upload_store(const vector<string> &value)
{
	if (value.size() != 1)
		throw runtime_error("upload_store in location block needs only one value");
	return (value[0]);
}

/**
 * @brief check si le prefixe path s'arrete sur une frontiere de segment de l'URI
 *
 * Evite qu'une location "/img" matche l'URI "/images/logo.png".
 * @pre uri doit avoir path en prefixe (verifie par ServerConfig::Resolve)
 * @pre path est non vide (garanti par parse_location)
 * @param uri chemin indique dans la requette HTTP 1.1, query/fragment deja retires
 * @param path chemin indique par le bloc location entrain detre verifier
 * @return true si match exact, si path finit par '/', ou si le caractere suivant dans l'uri est un '/'
*/
bool	is_segment_boundary(const string &uri, const string &path)
{
	if (uri.size() == path.size())
		return (true);
	if (path[path.size() - 1] == '/')
		return (true);
	return (uri[path.size()] == '/');
}

void	check_valid_path(const string &path)
{
	size_t	flag;

	if (path.empty())
		throw runtime_error("Missing PATH argument for location block");
	flag = path.find_first_of(LOC_NO_SUPPORTED);
	if (flag == 0)
	{
		ostringstream	oss;
		oss << "location modifiers " << path[0] << " are not supported";
		throw runtime_error(oss.str());
	}
	flag = path.find_first_of('/');
	if (flag != 0)
		throw runtime_error("location needs to start with '/'");
}
