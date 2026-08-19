#include "../../includes/Connection.hpp"

	/*===Canonical Form===*/
Connection::Connection(void) {}

Connection::~Connection(void) {}

Connection::Connection(int fd, size_t group_index) : _Fd(fd), _GroupIndex(group_index) {}

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
		this->_InBuf= src._InBuf;
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

void	Connection::setIpV4(string src){
	this->_IpV4 = src;
}

const string&	Connection::getIpV4() const{
	return this->_IpV4;
}

size_t Connection::getGroupIndex() const{
	return (this->_GroupIndex);
}

	/*===Member Function===*/

	/**
	 * @brief Call and check the return from recv()
	 * Add the buffer in _InBuf then feed the request, check the return from it
	 * and if the request is completed it will append in the fonction
	 * QueueOutput
	 *
	 * @return ssize_t
	 */
ssize_t	Connection::OnReadable(){
	_State = CONN_READING;
	char	buffer[BUFFER_SIZE];
	ssize_t r = recv(_Fd, buffer, sizeof(buffer), NULL);
	if (r == 0)
		_State = CONN_CLOSING;
	else if (r < 0){
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		else
			_State = CONN_CLOSING;
	}
	else{
		_InBuf.append(buffer, r);
		EParseResult res = _Req.Feed(_InBuf.c_str(), _InBuf.size());
		_InBuf.erase(0, r);
		if (res == REQ_ERROR)
			this->_State = CONN_CLOSING;
		else if (res == REQ_COMPLETE)
			QueueOutput("HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello world!\n");	//TODO (c-05)
	}
	return (r);
}

/**
 * @brief Call and check the return from send().
 * and erase _OutBuf with the size of send()
 *
 * @return ssize_t the size of the return from send()
 */
ssize_t Connection::OnWritable(){
	ssize_t s = send(_Fd, _OutBuf.c_str(), _OutBuf.size(), 0);
	if (s == 0)
		_State = CONN_CLOSING;
	else if (s < 0){
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return 0;
		else
			_State = CONN_CLOSING;
	}
	else
		_OutBuf.erase(0, s);
	return (s);
}

void Connection::QueueOutput(const string& data){
	_OutBuf.append(data);
	_State = CONN_WRITING;
}

bool Connection::HasPendingOutput() const{
	return !(_OutBuf.empty());
}
