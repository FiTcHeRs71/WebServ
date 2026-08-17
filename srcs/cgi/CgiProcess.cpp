#include "../../includes/CgiProcess.hpp"
#include <csignal>
#include <sched.h>
#include <unistd.h>
#include <fcntl.h>
#include <vector>

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
bool	CgiProcess::Start(const Request &request, const LocationConfig &location,
							const ServerConfig &server, const Connection &connection,
							const ConfigParser &config, const string &script_path)
{
	(void)request;
	char	*argv[3];
	char	**envp;
	int		pip_in[2];
	int		pip_out[2];

	argv[0] = const_cast<char *>(location.getPass().c_str());
	argv[1] = const_cast<char *>(script_path.c_str());
	argv[2] = NULL;
	vector<string>	storage = build_cgi_env(request, location, server, connection, config);
	envp = VectorToChar(storage);

	// Dossier du script pour le chdir() de l'enfant. Les deux cas limites
	// donnent un chemin invalide si on prend le substr tel quel :
	// "script.bla" (pas de '/') -> "" et "/script.bla" (slash en tete) -> "".
	size_t	slash = script_path.rfind('/');
	string	dir;

	if (slash == string::npos)
		dir = ".";
	else if (slash == 0)
		dir = "/";
	else
		dir = script_path.substr(0, slash);

	const char *dir_c = dir.c_str();
	if (!this->SetupPipes(pip_in, pip_out))
		return (false);

	pid_t	pid = fork();

	if(pid == -1)
	{
		close(pip_out[0]);
		close(pip_out[1]);
		close(pip_in[0]);
		close(pip_in[1]);
		return (false);
	}
	if (pid == 0)
	{
		chdir(dir.c_str());
		if (dup2(pip_in[0], STDIN_FILENO) < 0 || dup2(pip_out[1], STDOUT_FILENO) < 0)
			_exit(1);
		// Fermeture en force de tout ce qui est herite du serveur : sockets
		// d'ecoute, connexions des autres clients, et les extremites de pipe
		// deja dupliquees sur 0/1 juste au-dessus. FD_CLOEXEC n'est pas une
		// option : le sujet limite fcntl() a F_SETFL / O_NONBLOCK.
		for (int fd = 3; fd < 1024; fd++)
			close(fd);
		if (chdir(dir_c) < 0)
			_exit(1);
		execve(argv[0], argv, envp);
		_exit(1);
	}
	close(pip_in[0]);
	close(pip_out[1]);
	this->_WriteFd = pip_in[1];
	this->_ReadFd = pip_out[0];
	this->_Pid = pid;
	this->_StartTime = time(NULL);
	return (true);
}

/**
 * @brief Tue le processus CGI et ferme les pipes. Appele sur timeout (D-05).
 */
void	CgiProcess::Kill(void)
{
	if (this->_Pid > 0)
		kill(this->_Pid, SIGKILL);
	this->CloseFds();
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

/**
 * @brief Ferme les deux pipes et invalide les descripteurs.
 *
 * Idempotente : les fds sont remis a -1, un second appel ne fait rien.
 * A appeler par l'EventLoop quand le CGI sort du poll (B-07).
 * Le destructeur ne ferme PAS : voir le contrat en tete de classe.
 */
void	CgiProcess::CloseFds(void)
{
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
}

	/**
	 * @brief Ferme le stdin du CGI pour lui signaler la fin du body.
	 *
	 * Sans cet appel, le script reste bloque sur read(stdin) : le pipe
	 * a toujours un ecrivain vivant. A appeler des le dernier octet ecrit,
	 * y compris quand il n'y a pas de body (GET).
	 */
void	CgiProcess::CloseWriteFd(void)
{
		if (this->_WriteFd != -1)
	{
		close(this->_WriteFd);
		this->_WriteFd = -1;
	}
}