#ifndef CGI_PROCESS_HPP
# define CGI_PROCESS_HPP

# include "./LocationConfig.hpp"
# include "./ServerConfig.hpp"
#include "./Connection.hpp"
#include "./Config.hpp"
# include <ctime>
# include <iostream>
# include <sys/types.h>
# include <string>

using namespace std;

class Request;
class LocationConfig;

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
	time_t	_StartTime;	///< D-05 : timeout
	string	_InBuf;		///< body restant a ecrire (D-03)
	string	_OutBuf;	///< sortie brute accumulee (D-04)
	bool	_Finished;

	bool	SetupPipes(int pip_in[2], int pip_out[2]);

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

	/*===Member Function===*/
	bool	Start(const Request &request, const LocationConfig &location,
					const ServerConfig &server, const Connection &connection,
					const ConfigParser &config, const string &script_path);
	void	Kill(void);				///< D-05
	void	CloseFds(void);			///< Ferme les 2 pipes. Idempotente.
	void	CloseWriteFd(void);		///< Ferme le stdin du CGI : D-03, quand le body est entierement ecrit.
};

vector<string>	build_cgi_env(const Request &request, const LocationConfig &location,
					const ServerConfig &server, const Connection &connection, const ConfigParser &config);
char	**VectorToChar(vector<string> &storage);

#endif /*CGI_PROCESS_HPP*/