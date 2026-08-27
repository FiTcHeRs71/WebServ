#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include "./Request.hpp"
# include "./Response.hpp"
# include "./CgiProcess.hpp"
# include <iostream>
# include <sys/types.h>
# include <sys/socket.h>
#include <arpa/inet.h>

using namespace std;

/**
 * @brief Represente une connexion client acceptee sur un socket d'ecoute.
 *
 * Porte le socket du client ainsi que l'etat necessaire pour
 * recevoir sa requete, la faire traiter et lui renvoyer la reponse.
 */
enum EConnState
{
	CONN_READING,		///< on accumule la requete
	CONN_WRITING,		///< la reponse est prete, on vide _OutBuf
	CONN_CLOSING		///< a fermer des que _OutBuf est vide
};

class ConfigParser;

class Connection
{
	private:

	int				_Fd;
	Request			_Req;
	string			_IpV4;
	string			_OutBuf;		///< octets a envoyer, produits par Response::Serialize()
	EConnState		_State;
	time_t			_LastActivity;	///< utilise par B-05 pour les timeouts
	size_t			_GroupIndex;	///< index dans _AddrPorts : quel ServerConfig repond
	bool			_ReqComplete;
	CgiProcess		_Cgi;

	public:

	/*===Canonical Form===*/
	Connection(void);
	~Connection(void);
	Connection(const Connection& to_copy);
	Connection &operator=(const Connection& src);
	Connection(int fd, size_t group_index, const ConfigParser *config);

	/*===Getters & Setters===*/
	int				getFd(void) const;
	EConnState		getState() const;
	time_t			getLastActivity(void) const;
	void			setIpV4(string src);
	const string&	getIpV4() const;
	size_t			getGroupIndex() const;
	void			setLastActivity();
	bool			getReqComplete(void) const;
	Request&		getRequest();
	void			setState(EConnState state);
	CgiProcess&		getCgi();


	/*===Member Function===*/
	bool			HasPendingOutput(void) const;	///< pilote l'armement de POLLOUT
	ssize_t			OnReadable(void);				///< un seul recv(), renvoie ce que recv() a rendu
	ssize_t			OnWritable(void);				///< un seul send() partiel
	void			QueueOutput(const string& data);
	void			SendErrorAndClose(int code);
};

#endif /*CONNECTION_HPP*/
