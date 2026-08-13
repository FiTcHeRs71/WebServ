#include "../../includes/EventLoop.hpp"
#include <algorithm>
#include <condition_variable>
#include <poll.h>
#include <sys/poll.h>
#include <type_traits>
#include <vector>

EventLoop::EventLoop(const ConfigParser &config, const ListenSockets &sockets)
	:_Config(&config)
	,_Running(true)
{
	vector<int>				servFds = sockets.getServFd();

	for (size_t i = 0; i <servFds.size(); i++)
	{
		_ListenFds[servFds[i]] = i; ///< servFds[i] is the socket, i is the index in _AddrPorts
		AddFd(servFds[i], POLLIN);
	}

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
		status = poll(&_Pollfds[0], _Pollfds.size(), -1); // timeout = -1 might need to be dropped in B-05
		if (status == 0)
			continue ;
		else if (status < 0)
		{
			if (errno == EINTR) // errno is okay, subject says forbidden only after a read or write
				continue ;
			cerr << "Error: poll() failed." << endl;
			break ;
		}
		for (size_t i = 0; i < _Pollfds.size(); i++)
		{
			if (_Pollfds[i].revents == 0)
			{
				i++;
				continue ;
			}
			int	fd = _Pollfds[i].fd;
			if (_ListenFds.count(fd)) ///< 1 is a listen fd so we accept, 0 isn't, its a client so we handle
			{
				if (_Pollfds[i].revents & POLLIN)
					AcceptNewClients(fd);
				i++;
			}
			else
				HandleClientEvent(i);
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

