#include "../../includes/EventLoop.hpp"
#include <algorithm>
#include <condition_variable>
#include <poll.h>
#include <sys/poll.h>

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
	return (*this);
}


void	EventLoop::Run(void)
{
	int	status;
	while (_Running)
	{
		status = poll(&_Pollfds[0], _Pollfds.size(), -1);
		if (status == 0) // check what to do with in case of poll errors
		{
			cerr << "Poll error: timed out." << endl;
			return ;
		}
		else if (status == -1) // check what to do with in case of poll errors
		{
			cerr << "Poll error: failed." << endl;
			return ;
		}
		for (size_t i = 0; i < _Pollfds.size(); i++)
		{
			if ((_Pollfds[i].revents & POLLIN) || (_Pollfds[i].revents & POLLOUT))
			{
				//dispatch entre Accept...() et Handle...(), RemoveFd() en sortie de fonction
				//_Pollfds contient quoi ?
			}
		}
	}
}

int	EventLoop::FindFd(int fd)
{
	for (size_t i = 0; i < _Pollfds.size(); i++)
	{
		if (_Pollfds[i].fd == fd)
			return (static_cast<int>(i));
	}
	return (-1);
}

void	EventLoop::AddFd(int fd, short events)
{
	struct pollfd	newFd;
	int				idx;

	if (fd < 0)
		return ;
	idx = FindFd(fd);
	if (idx != -1)
	{
		_Pollfds[idx].events = events;
		return ;
	}
	newFd.fd = fd;
	newFd.events = events;
	newFd.revents = 0;
	_Pollfds.push_back(newFd);
}

void	EventLoop::RemoveFd(int fd)
{
	int	idx;

	if (fd < 0)
		return ;
	idx  = FindFd(fd);
	if (idx != -1)
	{
		_Pollfds.erase(_Pollfds.begin() + idx);
		return ;
	}
	cerr << "This fd wasnt found." << endl; //checker si message d'erreur necessaire ou si on skip.
}

void	EventLoop::SetEvents(int fd, short events)
{
	int				idx;

	if (fd < 0)
		return ;
	idx = FindFd(fd);
	if (idx != -1)
	{
		_Pollfds[idx].events = events;
		return ;
	}
	cerr << "This fd wasnt found." << endl; //checker si message d'erreur necessaire ou si on skip.
}

void	EventLoop::AcceptNewClients(int listen_fd)
{

}

void	EventLoop::HandleClientEvent(size_t index)
{

}

