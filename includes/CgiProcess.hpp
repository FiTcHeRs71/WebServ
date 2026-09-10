#ifndef CGI_PROCESS_HPP
# define CGI_PROCESS_HPP

# include "./LocationConfig.hpp"
# include "./ServerConfig.hpp"
# include "./Config.hpp"
# include <ctime>
# include <iostream>
# include <sys/types.h>
# include <string>

using namespace std;

class Request;
class LocationConfig;
class Connection;
class Response;

/**
 * @brief Gere le cycle de vie d'un processus CGI externe.
 *
 * Ownership des fds : le destructeur ne ferme rien, volontairement.
 * La forme canonique copie _ReadFd/_WriteFd ; un destructeur fermant
 * provoquerait un double close des qu'un conteneur copierait l'objet.
 * C'est donc a l'appelant (EventLoop, B-07) d'appeler CloseFds().
 */
class CgiProcess
{
	private:

	pid_t	_Pid;
	int		_ReadFd;
	int		_WriteFd;
	time_t	_LastIo;	///< derniere I/O reussie avec le CGI : le timeout est un timeout d'inactivite
	time_t	_StartTime;	///< when the cgi process start used for waitpid with default_cgitimeout
	string	_InBuf;		///< body restant a ecrire (D-03)
	size_t	_InOff;		///< octets deja ecrits dans _InBuf (evite erase(0,n) en O(n^2))
	string	_OutBuf;	///< sortie brute accumulee (D-04)
	bool	_Finished;

	bool		SetupPipes(int pip_in[2], int pip_out[2]);

	public:

	/*===Canonical Form===*/
	CgiProcess(void);
	~CgiProcess(void);
	CgiProcess(const CgiProcess& to_copy);
	CgiProcess &operator=(const CgiProcess& src);

	/*===Getters & Setters===*/
	int		GetReadFd(void) const;
	int		GetWriteFd(void) const;
	pid_t	GetPid(void) const;
	const string&	GetOutBuf() const;	///< par reference : la sortie CGI peut peser 100 Mo
	void			ClearOutBuf();		///< libere la sortie brute des qu'elle est parsee

	/*===Member Function===*/
	bool	Start(const Request &request, const LocationConfig &location,
				const ServerConfig &server, const Connection &connection,
				const ConfigParser &config, const string &script_path);
	bool	IsTimedOut(time_t now) const; ///< now = _StartTime > CGI_TIMEOUT
	bool	Reap(int &exit_status); ///< waitpid
	void	Kill(void);				///< SIGKILL puis Reap()
	void	CloseFds(void);			///< Ferme les 2 pipes. Idempotente.
	void	OnWritableCgi(void);
	void	CloseWriteFd(void);		///< Ferme le stdin du CGI : D-03, quand le body est entierement ecrit.
	void	OnReadableCgi();///< Ferme le stdin du CGI : D-03, quand le body est entierement ecrit.
	void	CloseReadFd();
};

vector<string>	build_cgi_env(const Request &request, const LocationConfig &location,
							const ServerConfig &server, const Connection &connection,
							const ConfigParser &config, const string &script_path);
char			**VectorToChar(vector<string> &storage);
bool			parse_cgi_output(const std::string &raw, Response &out);
string			findCgiExt(const string &path, const LocationConfig &loc);	///< ext CGI dans path, ou ""

#endif /*CGI_PROCESS_HPP*/
