#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <iostream>
# include <string>

class Response
{
	private:



	protected:



	public:

	/*===Canonical Form===*/
	Response(void);
	~Response(void);
	Response(const Response& to_copy);
	Response &operator=(const Response& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/
	bool	serialize(std::string &out);
};

#endif /*RESPONSE_HPP*/