#include "../../includes/Connection.hpp"

	/*===Canonical Form===*/
Connection::Connection(void)
{
	//std::cout << "Connection default constructor called" << std::endl;
}

Connection::Connection(int fd, size_t group_index) :
_Fd(fd),
_State(CONN_READING),
_GroupIndex(group_index) {}

Connection::Connection(const Connection& to_copy)
{
	(void)to_copy;
	//std::cout << "Connection copy constructor called" << std::endl;
}

Connection	&Connection::operator=(const Connection& src)
{
	(void)src;
	//std::cout << "Connection operator assignement (=) constructor called" << std::endl;
	if (this != &src)
	{
		return (*this);
	}
	return (*this);
}

	/*===Getters & Setters===*/
int	Connection::getFd(void) const
{
	return(this->_Fd);
}

void	Connection::setIpV4(string src){
	(this->_IpV4 = src);
}

const string&	Connection::getIpV4() const{
	return (this->_IpV4);
}

size_t Connection::getGroupIndex() const{
	return (this->_GroupIndex);
}


EConnState Connection::getState() const{
	return (this->_State);
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
	ssize_t r = recv(_Fd, buffer, sizeof(buffer), 0);
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
		// {
		// 	string body(5 * 1024 * 1024, 'A');   // 5 Mo de 'A'
		// 	ostringstream oss;					///<
		// 	oss << body.size();
		// 	QueueOutput("HTTP/1.1 200 OK\r\nContent-Length: " + oss.str() + "\r\n\r\n" + body);
		// }
			QueueOutput("HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello world!\n");	//TODO (c-05)
	}
	return (r);
}

std::string	Connection::getIpv4(void) const
{
	return(_IpV4);
}

/**
 * @brief append in the string _OutBuf when the request in done then change the
 * State for Writing connection
 *
 * @param data The return from the request
 */
void Connection::QueueOutput(const string& data){
	_OutBuf.append(data);
	_State = CONN_WRITING;
}

/**
 * @brief
 *
 * @return true
 * @return false
 */
bool Connection::HasPendingOutput() const{
	return !(_OutBuf.empty());
}
