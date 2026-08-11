# include "../../includes/ListenSockets.hpp"

	/*===Canonical Form===*/
ListenSockets::ListenSockets(const vector<TAddrPortGroup>& AddrPorts)
{
	for (size_t i = 0; i < AddrPorts.size(); i++){
		const TAddrPortGroup& tmp = AddrPorts[i];
		if (!creatSocket(tmp.Host, tmp.Port)){
			closeFd();
			ostringstream ss;
			ss << "bind() failed on " << tmp.Host << ":"
			<< tmp.Port << " ";
			throw runtime_error(ss.str() + string(strerror(errno)));
		}
	}
}

ListenSockets::~ListenSockets(void)
{
	if (!this->_ServFd.empty())
		this->closeFd();
}

ListenSockets::ListenSockets(const ListenSockets& to_copy)
{
	for(size_t i = 0; i < to_copy._ServFd.size(); i++){
		int newFd = dup(to_copy._ServFd[i]);
		if (newFd < 0){
			this->closeFd();
			throw runtime_error("Error: " + string(strerror(errno)));
		}
		this->_ServFd.push_back(newFd);
	}
}

ListenSockets	&ListenSockets::operator=(const ListenSockets& src)
{
	if (this != &src){
		if (!this->_ServFd.empty())
			this->closeFd();
		for(size_t i = 0; i < src._ServFd.size(); i++){
			int newFd = dup(src._ServFd[i]);
			if (newFd < 0){
				this->closeFd();
				throw runtime_error("Error: " + string(strerror(errno)));
			}
			this->_ServFd.push_back(newFd);
		}
	}
	return (*this);
}


/*===Member Function===*/
/**
 * @brief Fonctions pour ouvrir les fd des sockets, 1 fd par port ouvert
 * stock des fds dans l'attribut>_ServFd
 *
 */
bool	ListenSockets::creatSocket(const string& Host, const int& Port){
	struct addrinfo addr;
	struct addrinfo *res;
	bzero(&addr, sizeof(addr));
	ostringstream ss;
	ss << Port;
	string s_port = ss.str();
	addr.ai_family = AF_INET;
	addr.ai_socktype = SOCK_STREAM;
	addr.ai_flags = AI_PASSIVE;

	// Permet de creer la structure de donnee pour ouvrir le fd
	///!\ Attention il faut free la structure avec freeaddrinfo
	if (getaddrinfo(Host.c_str(), s_port.c_str() , &addr, &res) != 0)
		return false;
	int opt = 1;
	int fd = socket(res->ai_family, res->ai_socktype, 0);
	if (fd < 0){
		freeaddrinfo(res);
		return false;
	}
	this->_ServFd.push_back(fd);

	// Permet de manipuler les options du socket , evite les erreurs tel que
	// "address already in use" si on quitte le programme et relance d'aussi tot
	if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0){
		freeaddrinfo(res);
		return false;
	}

	// Permet de lier une adresse local et le numero de port specifie par
	// addr au socket
	if (bind(fd, res->ai_addr, res->ai_addrlen) < 0){
		freeaddrinfo(res);
		return false;
	}

	// Permet de mettre le socket en mode en mode passif, attendant qu'un client
	// tente de se connecter. le parametre backlog defini la taille de la file d'attente
	if (listen(fd, BACKLOG) < 0){
		freeaddrinfo(res);
		return false;
	}
	//dis au kernel : "sur ce fd, si une opération (accept, read, write...)
	// n'a rien à faire immédiatement
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0){
		freeaddrinfo(res);
		return false;
	}
	freeaddrinfo(res);
	return true;
}

/**
 * @brief Permet d'iterer sur le container contenant les fds et de les close()
 *
 */
void ListenSockets::closeFd(){
	for(size_t i = 0; i < this->_ServFd.size(); i++){
		if (this->_ServFd[i])
			close(this->_ServFd[i]);
	}
}

	/*===Getters & Setters===*/
vector<int> ListenSockets::getServFd() const{
	return this->_ServFd;
}


	/*===Operateur de flux===*/
ostream& operator<<(ostream& flux, const ListenSockets& listen){

	for(size_t i = 0; i < listen._ServFd.size(); i++){
		flux << "ListenSockets._servFd[" << i << "] = ";
		flux << listen._ServFd[i] << endl;
	}
	return (flux);
}
