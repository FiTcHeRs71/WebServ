#include "../../includes/CgiProcess.hpp"
#include <csignal>
#include <unistd.h>
#include <fcntl.h>

	/*===Canonical Form===*/
CgiProcess::CgiProcess(void)
	:_Pid(0)
	,_ReadFd(-1)
	,_WriteFd(-1)
	,_StartTime(0)
	,_Finished(false)
{
	//std::cout << "LocationConfig default constructor called" << std::endl;
}

CgiProcess::~CgiProcess(void)
{
	//std::cout << "LocationConfig default destructor called" << std::endl;
}

CgiProcess::CgiProcess(const CgiProcess& to_copy)
	:_Pid(to_copy._Pid)
	,_ReadFd(to_copy._ReadFd)
	,_WriteFd(to_copy._WriteFd)
	,_StartTime(to_copy._StartTime)
	,_InBuf(to_copy._InBuf)
	,_OutBuf(to_copy._OutBuf)
	,_Finished(to_copy._Finished)
{
	//std::cout << "LocationConfig copy constructor called" << std::endl;
}
CgiProcess	&CgiProcess::operator=(const CgiProcess& src)
{
	//std::cout << "LocationConfig operator assignement(=) constructor called" << std::endl;
	if (this != &src)
	{
		this->_Pid = src._Pid;
		this->_ReadFd = src._ReadFd;
		this->_WriteFd = src._WriteFd;
		this->_StartTime = src._StartTime;
		this->_InBuf = src._InBuf;
		this->_OutBuf = src._OutBuf;
		this->_Finished = src._Finished;
	}
	return (*this);
}

	/*===Getters & Setters===*/
	/**
	 * @brief Retourne le descripteur de lecture du pipe vers le CGI.
	 * @return Le file descriptor en lecture, ou -1 si non disponible.
	 */
int	CgiProcess::GetReadFd(void) const
{
	return (this->_ReadFd);
}

	/**
	 * @brief Retourne le descripteur d'ecriture du pipe vers le CGI.
	 * @return Le file descriptor en ecriture, ou -1 si non disponible.
	 */
int	CgiProcess::GetWriteFd(void) const
{
	return (this->_WriteFd);
}

	/**
	 * @brief Retourne le PID du processus CGI
	 * @return Le Pid, ou 0 si aucun processus n'a ete lance
	 */
pid_t	CgiProcess::GetPid(void) const
{
	return (this->_Pid);
}

	/*===Member Function===*/
	/**
 * @brief Demarre l'execution du script CGI correspondant a la requete.
 * @param request  La requete HTTP a transmettre au processus CGI.
 * @param location La configuration de location associee (chemin du CGI, etc.).
 * @param script_path Le path vers le script a executer
 */
bool	CgiProcess::Start(const Request &request, const LocationConfig &location, const string &script_path)
{
	(void)request;
	(void)location;
	(void)script_path;
	return (false);
}

	/**
	 * @brief Tue le processus CGI et ferme les pipes. Appele sur timeout (D-05).
	 */
void	CgiProcess::Kill(void)
{
	if (this->_Pid > 0)
		kill(this->_Pid, SIGKILL);
	if (this->_ReadFd != -1)
	{
		close(this->_ReadFd);
		this->_ReadFd = -1;
	}
	if (this->_WriteFd != -1)
	{
		close(this->_WriteFd);
		this->_WriteFd = -1;
	}
	this->_Finished = true;
}

/**
 * @brief Cree les deux pipes de communication avec le futur processus CGI.
 *
 * @param pipe_in  [out] pipe serveur -> CGI : [0] lu par l'enfant (stdin),  [1] ecrit par le parent
 * @param pipe_out [out] pipe CGI -> serveur : [0] lu par le parent,         [1] ecrit par l'enfant (stdout)
 * @return true si les deux pipes sont prets, false sinon (rien n'est laisse ouvert).
 */
bool	CgiProcess::SetupPipes(int pip_in[2], int pip_out[2])
{
	if (pipe(pip_in) == -1)
		return (false);
	if (pipe(pip_out) == -1)
	{
		close(pip_in[0]);
		close(pip_in[1]);
		return (false);
	}
	if (fcntl(pip_in[1], F_SETFL, O_NONBLOCK) == -1 || fcntl(pip_out[0], F_SETFL, O_NONBLOCK) == -1)
	{
		close(pip_out[0]);
		close(pip_out[1]);
		close(pip_in[0]);
		close(pip_in[1]);
		return (false);
	}
	return (true);
}