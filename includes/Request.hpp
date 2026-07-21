#ifndef REQUEST_HPP
# define REQUEST_HPP

# include <iostream>
# include <cstddef>

class Request
{
	private:



	protected:



	public:

	/*===Canonical Form===*/
	Request(void);
	~Request(void);
	Request(const Request& to_copy);
	Request &operator=(const Request& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/
	int	Feed(const char *data, size_t n);
};

#endif /*REQUEST_HPP*/