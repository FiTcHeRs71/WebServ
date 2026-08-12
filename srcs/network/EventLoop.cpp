#include "../../includes/EventLoop.hpp"

EventLoop::EventLoop(const ConfigParser &config, const ListenSockets &sockets)
	:_Config(&config)
{
	vector<TAddrPortGroup>	AddrPorts = config.getAddrPorts();
	size_t fd_i = 0;
	for (size_t i = 0; i < AddrPorts.size(); i++)
	{
		for(size_t j = 0; j < AddrPorts[i].ServerIndexes.size(); j++)
		{
			_ListenFds[fd_i] = AddrPorts[i].ServerIndexes[j];
			fd_i++;
		}
	}
	_Running = true;
	cout << "EventLoop constructor called." << endl;
}

EventLoop::~EventLoop(void)
{
	cout << "EventLoop destructor called." << endl;
}

EventLoop::EventLoop(const EventLoop& to_copy)
	:_Pollfds(to_copy._Pollfds)
	,_Clients(to_copy._Clients)
	,_ListenFds(to_copy._ListenFds)
	,_Config(to_copy._Config)
	,_Running(to_copy._Running)
{
	cout << "EventLoop copy constructor called." << endl;
}

EventLoop &EventLoop::operator=(const EventLoop& src)
{
	if (this != &src)
	{
		this->_Pollfds = src._Pollfds;
		this->_Clients = src._Clients;
		this->_ListenFds = src._ListenFds;
		this->_Config = src._Config;
		this->_Running = src._Running;
	}
	cout << "EventLoop copy assignment operator called." << endl;
}


void	EventLoop::Run(void)
{

}

void	EventLoop::AddFd(int fd, short events)
{
	struct pollfd	newFd;

	if (fd < 0 || events < 0)
		return ;
	newFd.fd = fd;
	newFd.events = events;
	_Pollfds.push_back(newFd);
}

void	EventLoop::RemoveFd(int fd)
{

}

void	EventLoop::SetEvents(int fd, short events)
{

}

void	EventLoop::AcceptNewClients(int listen_fd)
{

}

void	EventLoop::HandleClientEvent(size_t index)
{

}

