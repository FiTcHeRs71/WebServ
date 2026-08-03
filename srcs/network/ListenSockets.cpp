# include "../../includes/ListenSockets.hpp"

	/*===Canonical Form===*/
ListenSockets::ListenSockets(const std::vector<ServerConfig> &servers)
{
	for (size_t i = 0; i < servers.size(); i++){
		const vector<pair <string, int> > listens = servers[i].getListens();
		for(size_t j = 0; j < listens.size(); j++){
		if (!creatSocket(listens[j]))
			throw runtime_error("bind() failed on " + listens[j].first);
		}
	}
	//std::cout << "ListenSockets default constructor called" << std::endl;
}

ListenSockets::~ListenSockets(void)
{
	//std::cout << "ListenSockets default destructor called" << std::endl;
}

ListenSockets::ListenSockets(const ListenSockets& to_copy)
{
	(void)to_copy;
	//std::cout << "ListenSockets copy constructor called" << std::endl;
}

ListenSockets	&ListenSockets::operator=(const ListenSockets& src)
{
	(void)src;
	//std::cout << "ListenSockets assignement operator(=) constructor called" << std::endl;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}


	/*===Getters & Setters===*/

/**
 * @brief Fonctions pour ouvrir les fd des sockets, 1 fd par port ouvert
 * stock des fds dans les attributs privees
 *
 */
	/*===Member Function===*/
bool	ListenSockets::creatSocket(const pair<string, int>& listens){
	struct addrinfo addr;
	struct addrinfo *res;
	bzero(&addr, sizeof(addr));
	ostringstream ss;
	ss << listens.second;
	string port = ss.str();
	addr.ai_family = AF_INET;
	addr.ai_socktype = SOCK_STREAM;
	addr.ai_flags = AI_PASSIVE;
	getaddrinfo(listens.first.c_str(), port.c_str() , &addr, &res);
	int fd = socket(res->ai_family, res->ai_socktype, 0);
	if (fd < 0)
		return false;
	else
		this->_sockFd.push_back(fd);
	return true;
}
