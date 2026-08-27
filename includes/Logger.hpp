#ifndef LOGGER_HPP
# define LOGGER_HPP

# include <iostream>
# include <fstream>
# include <streambuf>
# include <string>

using namespace std;

/**
 * @brief Detourne cout et cerr vers un fichier .log, en RAII.
 *
 * On remplace le streambuf sur lequel cout et cerr pointent : aucun cout du
 * projet n'est a reecrire, c'est la destination qui bouge. Bascule a la
 * construction, restauration a la destruction.
 *
 * @warning Un seul Logger doit exister : deux instances redirigees
 * restaureraient des buffers incoherents. La copie est donc neutralisee.
 */
class Logger
{
	private:

	ofstream	_File;			///< Doit survivre tant que cout pointe sur son streambuf
	streambuf	*_SavedCout;	///< Buffer d'origine de cout (stdout), a restaurer
	streambuf	*_SavedCerr;	///< Buffer d'origine de cerr (stderr), a restaurer
	bool		_Redirected;	///< false si l'ouverture a echoue : rien a defaire

	public:

	/*===Canonical Form===*/
	Logger(void);							///< Aucune redirection, membres neutres
	Logger(const string &path);				///< Ouvre path en append et bascule cout/cerr dessus
	~Logger(void);							///< Restaure cout/cerr PUIS ferme le fichier
	Logger(const Logger& to_copy);			///< Copie neutre : ne duplique pas la redirection
	Logger&operator=(const Logger& src);	///< Idem, la ressource n'est jamais partagee

	/*===Member Function===*/
	void	write(const string &level, const string &msg);	///< Ligne horodatee "date [level] msg"
};

#endif
