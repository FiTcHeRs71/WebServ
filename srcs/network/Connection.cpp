#include "../../includes/Connection.hpp"

	/*===Canonical Form===*/
Connection::Connection(void) {}

Connection::~Connection(void) {}

Connection::Connection(const Connection& to_copy)
{
	this->_Fd = to_copy._Fd;
	this->_Req = to_copy._Req;
	this->_InBuf = to_copy._InBuf;
	this->_OutBuf = to_copy._OutBuf;
	this->_State = to_copy._State;
	this->_LastActivity = to_copy._LastActivity;
	this->_GroupIndex = to_copy._GroupIndex;
}

Connection	&Connection::operator=(const Connection& src)
{
	if (this != &src)
	{
		this->_Fd = src._Fd;
		this->_Req = src._Req;
		this->_InBuf = src._InBuf;
		this->_OutBuf = src._OutBuf;
		this->_State = src._State;
		this->_LastActivity = src._LastActivity;
		this->_GroupIndex = src._GroupIndex;
	}
	return (*this);
}

	/*===Getters & Setters===*/
int	Connection::getFd(void) const
{
	return(this->_Fd);
}

	/*===Member Function===*/

size_t	Connection::OnReadable(){
	char	buffer[4096];
	while(size_t len = recv(this->_Fd, buffer , sizeof(buffer), NULL) > 0){	///!< Flag a definir (non bloquant ?)
		if (len < 0){
			cerr << "Error: " << strerror(errno) << endl;
		}
		this->_InBuf += buffer;
	}
}
