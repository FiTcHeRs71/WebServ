#include "../../includes/Connection.hpp"
#include "../../includes/http.hpp"
#include <ctime>
#include <string>

	/*===Canonical Form===*/
Connection::Connection(void) :
_Fd(0),
_State(CONN_READING),
_LastActivity(time(NULL)),
_GroupIndex(0) {}

Connection::~Connection(void) {}

Connection::Connection(int fd, size_t group_index, const ConfigParser *config)
	:_Fd(fd),
	_State(CONN_READING)
	,_LastActivity(time(NULL))
	,_GroupIndex(group_index)
{
	_Req.SetConnectionContext(config, group_index);
}

Connection::Connection(const Connection& to_copy)
{
	this->_Fd = to_copy._Fd;
	this->_Req = to_copy._Req;
	this->_IpV4 = to_copy._IpV4;
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
		this->_IpV4 = src._IpV4;
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

time_t Connection::getLastActivity() const{
	return (this->_LastActivity);
}
	/*===Member Function===*/

/**
 * @brief Call and check the return from recv()
 * feed the Request::feed buffer with octet sended, check the return from it
 * and if the request is completed it will append in the fonction
 * QueueOutput
 *
 * @return ssize_t
 */
ssize_t	Connection::OnReadable(){
	_LastActivity = time(NULL);
	char	buffer[BUFFER_SIZE];
	ssize_t r = recv(_Fd, buffer, sizeof(buffer), 0);
	if (r <= 0){
			_State = CONN_CLOSING;
			return (r);
	}
	else{
		EParseResult	res = _Req.Feed(buffer, r);
		while (res == REQ_COMPLETE){
			string	out;
			const ServerConfig *srv = _Req.getServerConfig();
			if (srv == NULL)
			{
				_State = CONN_CLOSING;
				break ;
			}
			Response	rep = HandleRequest(_Req, *srv, *this);
			rep.Serialize(out);
			QueueOutput(out);
			_Req.reset();
			res = _Req.Feed("", 0);
		}
		if (res == REQ_ERROR)
			this->_State = CONN_CLOSING;
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
	_LastActivity = time(NULL);
	if (_OutBuf.empty())
		return (0);
	ssize_t s = send(_Fd, _OutBuf.c_str(), _OutBuf.size(), 0);
	if (s <= 0){
			_State = CONN_CLOSING;
			return (s);
	}
	else
		_OutBuf.erase(0, s);
	return (s);
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
 * @brief booleen for know if we have to Re-arme POLLOUT
 *
 * @return true	_OutBuf is ready
 * @return false _Outbuf is empty
 */
bool Connection::HasPendingOutput() const{
	return !(_OutBuf.empty());
}
