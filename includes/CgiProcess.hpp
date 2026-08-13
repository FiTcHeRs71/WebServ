#ifndef CGI_PROCESS_HPP
# define CGI_PROCESS_HPP

# include "./LocationConfig.hpp"
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
 * Encapsule le lancement du script CGI (fork/exec) ainsi que les
 * descripteurs de fichiers utilises pour communiquer avec lui.
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
	bool	Start(const Request &request, const LocationConfig &location, const string &script_path);
	void	Kill(void);	///< D-05
};

#endif /*CGI_PROCESS_HPP*/