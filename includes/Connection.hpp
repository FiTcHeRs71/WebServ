#ifndef CONNECTION_HPP
# define CONNECTION_HPP

# include <iostream>

/**
 * @brief Represente une connexion client acceptee sur un socket d'ecoute.
 *
 * Porte le socket du client ainsi que l'etat necessaire pour
 * recevoir sa requete, la faire traiter et lui renvoyer la reponse.
 */
class Connection
{
	private:



	protected:



	public:

	/*===Canonical Form===*/
	Connection(void);
	~Connection(void);
	Connection(const Connection& to_copy);
	Connection &operator=(const Connection& src);

	/*===Getters & Setters===*/


	/*===Member Function===*/

};

#endif /*CONNECTION_HPP*/