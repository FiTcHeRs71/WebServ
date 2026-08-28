#include "../../includes/Logger.hpp"
#include <ctime>
#include <iostream>

/*===Canonical Form===*/
/**
 * @brief Logger inactif. _Redirected a false : le destructeur ne touchera pas
 * aux pointeurs non initialises.
 */
Logger::Logger(void)
	:_SavedCout(NULL)
	,_SavedCerr(NULL)
	,_Redirected(false)
{}

/**
 * @brief Ouvre le fichier et bascule cout et cerr dessus.
 *
 * Sauvegarder les streambuf avant de les remplacer
 * ios::app cree le fichier s'il manque et ecrit a la fin
 *
 * @note Un echec d'ouverture n'est pas fatal : warning sur cerr (encore au
 * terminal a cet instant) et le serveur demarre, sortie inchangee.
 *
 * @param path Chemin du fichier, pas un repertoire : ofstream ne cree pas les
 * dossiers intermediaires.
 */
Logger::Logger(const string &path)
	:_SavedCout(NULL)
	,_SavedCerr(NULL)
	,_Redirected(false)
{
	this->_File.open(path.c_str(), ios::out|ios::app);
	if (!this->_File.is_open())
	{
		cerr << "webserv: [warn] cannot open " << path << ", logging to terminal" << endl;
		return ;
	}
	this->_SavedCout = cout.rdbuf();
	this->_SavedCerr = cerr.rdbuf();
	cout.rdbuf(this->_File.rdbuf());
	cerr.rdbuf(this->_File.rdbuf());
	this->_Redirected = true;
}

/**
 * @brief Rend cout et cerr au terminal, puis ferme le fichier.
 *
 * @warning Restaurer AVANT de fermer. Dans l'autre sens, cout pointerait sur
 * un streambuf mort et le premier cout suivant serait un comportement
 * indefini.
 */
Logger::~Logger(void)
{
	if (!this->_Redirected)
		return;
	cout.rdbuf(this->_SavedCout);
	cerr.rdbuf(this->_SavedCerr);
	_File.close();
}

/**
 * @brief Copie neutre : _Redirected force a false.
 *
 * Sinon la copie se croirait proprietaire et son destructeur rendrait cout au
 * terminal alors que l'original est vivant : log coupe en plein run. Le
 * ofstream n'est de toute facon pas copiable en C++98.
 */
Logger::Logger(const Logger& to_copy)
	:_SavedCout(to_copy._SavedCout)
	,_SavedCerr(to_copy._SavedCerr)
	,_Redirected(false)
{}

/**
 * @brief Affectation neutre, meme raison.
 *
 * @warning Affecter sur un Logger deja redirige perdrait ses streambuf
 * d'origine : plus personne ne restaurerait cout.
 */
Logger	&Logger::operator=(const Logger& src)
{
	if (this != &src)
	{
		this->_SavedCout = src._SavedCout;
		this->_SavedCerr = src._SavedCerr;
		this->_Redirected = false;
	}
	return (*this);
}

	/*===Member Function===*/

/**
 * @brief Ecrit "27/08/2026 14:16:19 [info] listening on 0.0.0.0:8080".
 *
 * Passe par cout et non par _File : cout etant deja detourne la ligne part
 * dans le fichier, le flush permet le tail -f
 *
 * @param level "info", "warn" ou "error".
 * @param msg Message deja formate par l'appelant.
 */
void	Logger::write(const string &level, const string &msg)
{
	time_t		now = time(NULL);
	struct tm	*t = localtime(&now);
	char stamp[20];		///< 19 caracteres de date + le '\0'

	strftime(stamp, sizeof(stamp), "%d/%m/%Y %H:%M:%S", t);
	cout << stamp << " [" << level << "] " << msg << endl;
}
