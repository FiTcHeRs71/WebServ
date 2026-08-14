#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <iostream>

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

class Connection
{
	private:

	int				_Fd;
	std::string		_InBuf;			///< octets recus, consommes par Request::Feed()
	std::string		_OutBuf;		///< octets a envoyer, produits par Response::Serialize()
	EConnState		_State;
	time_t			_LastActivity;	///< utilise par B-05 pour les timeouts
	size_t			_GroupIndex;	///< index dans _AddrPorts : quel ServerConfig repond

	protected:

	public:
	
	/*===Canonical Form===*/
	Connection(void);
	~Connection(void);
	Connection(const Connection& to_copy);
	Connection &operator=(const Connection& src);
	
	/*===Getters & Setters===*/
	int			getFd(void) const;
	bool		HasPendingOutput(void) const;	///< pilote l'armement de POLLOUT
	time_t		getLastActivity(void) const;
	
	
	/*===Member Function===*/
	size_t		OnReadable(void);				///< un seul recv(), renvoie ce que recv() a rendu
	size_t		OnWritable(void);				///< un seul send() partiel
	void		QueueOutput(const std::string &data);

};

#endif /*CONNECTION_HPP*/