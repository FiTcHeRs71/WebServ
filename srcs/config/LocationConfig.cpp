#include "../../includes/LocationConfig.hpp"
#include "../../includes/Config.hpp"
#include <set>
#include <stdexcept>

	/*===Canonical Form===*/
LocationConfig::LocationConfig(void)
	:_AutoIndex(false)
	,_ReturnCode(-1)
	,_HasReturn(false)
	,_ClientMaxBodySize(0)
	,_HasClientMaxBodySize(false)
	,_HasUploadStore(false)
{
	//std::cout << "LocationConfig default constructor called" << std::endl;
}

LocationConfig::~LocationConfig(void)
{
	//std::cout << "LocationConfig default destructor called" << std::endl;
}

LocationConfig::LocationConfig(const LocationConfig& to_copy)
	:_Path(to_copy._Path)
	,_Methods(to_copy._Methods)
	,_Root(to_copy._Root)
	,_Index(to_copy._Index)
	,_AutoIndex(to_copy._AutoIndex)
	,_ReturnCode(to_copy._ReturnCode)
	,_HasReturn(to_copy._HasReturn)
	,_CgiExt(to_copy._CgiExt)
	,_CgiPass(to_copy._CgiPass)
	,_ClientMaxBodySize(to_copy._ClientMaxBodySize)
	,_HasClientMaxBodySize(to_copy._HasClientMaxBodySize)
	,_ReturnTarget(to_copy._ReturnTarget)
	,_UploadStore(to_copy._UploadStore)
	,_HasUploadStore(to_copy._HasUploadStore)
{
	//std::cout << "LocationConfig copy constructor called" << std::endl;
}
LocationConfig	&LocationConfig::operator=(const LocationConfig& src)
{
	//std::cout << "LocationConfig assignement operator(=) constructor called" << std::endl;
	if (this != &src)
	{
		this->_Path = src._Path;
		this->_Methods = src._Methods;
		this->_Root = src._Root;
		this->_Index = src._Index;
		this->_AutoIndex = src._AutoIndex;
		this->_ReturnCode = src._ReturnCode;
		this->_HasReturn = src._HasReturn;
		this->_CgiExt = src._CgiExt;
		this->_CgiPass = src._CgiPass;
		this->_ClientMaxBodySize = src._ClientMaxBodySize;
		this->_HasClientMaxBodySize = src._HasClientMaxBodySize;
		this->_ReturnTarget = src._ReturnTarget;
		this->_UploadStore = src._UploadStore;
		this->_HasUploadStore = src._HasUploadStore;
	}
	return (*this);
}

	/*===Getters & Setters===*/

	/*===Member Function===*/
/**
 * @brief Surcharge d'operateur pour l'impression des attributs de la classe LocationConfig
 *
 * @return le flux rempli
 */
ostream	&operator<<(ostream &flux, const LocationConfig &src)
{
	flux << "=== Location block ===" << endl;
	flux << "Path = " << src._Path << endl;
	flux << "Methods = ";
	set<string>::const_iterator it;
	for (it = src._Methods.begin(); it != src._Methods.end(); ++it)
		flux  << *it << " | ";
	flux << endl;
	flux << "Root = " << src._Root << endl;
	flux << "Index = ";
	for (size_t i = 0; i < src._Index.size(); i++)
		flux << src._Index[i] << " | ";
	flux << endl;
	flux << "Auto index = " << src._AutoIndex << endl;
	flux << "Return Code = " << src._ReturnCode << endl;
	flux << "Has return = " << src._HasReturn << endl;
	flux << "CGI ext = " << src._CgiExt << endl;
	flux << "CGI pass = " << src._CgiPass << endl;
	flux << "Client max body size = " << src._ClientMaxBodySize << endl;
	flux << "Has client max body size = " << src._HasClientMaxBodySize << endl;
	flux << "Return = " << src._ReturnTarget << endl;
	flux << "Upload_store = " << src._UploadStore << endl;
	flux << "Has Upload store = " << src._HasUploadStore << endl;
	flux << "=====================" << endl;
	return (flux);
}

/**
 * @brief Fonction dentre pour le parsing des block location contenu dans chaque bloc serveur
 * 
 * Elle remplie les attributs de l'objet LocationConfig avec les elements indique dans le bloc location
 * Effectue une verification du nombre darguments et leur validite pour chaque unstructions <KEY>
 * Les verifications se sont sur des criteres basic mais aucune verication sur des doublons ou des ports non disponible
 * @return le flux rempli
 */
void	LocationConfig::parse_location(vector<string>	&token, size_t &i)
{
	size_t flag = token[i].find_first_of("/");
	set<string>	seen;
	
	if (flag != 0)
		throw runtime_error(token[i] + " is not a valid PATH");
	this->_Path = token[i];
	i += 2; // saute le PATH + "{"
	while (i < token.size() && token[i] != "}")
	{
		string	key = token[i];
		i++;

		if (!seen.insert(key).second)
			throw runtime_error("Multiple definition of " + key + " not allowed in same location blocks");

		vector<string>	value = collect_values(token, i);
		if (key == "allow_methods")
		{
			/*if (!this->_Methods.empty())
				throw runtime_error("Multiple definition of allow_methods in location blocks");*/
			this->_Methods = parse_allow_methods(value);
		}
		else if (key == "root")
		{
			/*if (!this->_Root.empty())
				throw runtime_error("Invalid multiple definition of root in location bocks");*/
			this->_Root = parse_root(value);
		}
		else if (key == "index")
		{
			/*if (!this->_Index.empty())
				throw runtime_error("Multiple definition of index in location blocks");*/
			this->_Index = parse_index(value);
		}
		else if (key == "autoindex")
		{
			/*if (this->_AutoIndex)
				throw runtime_error("Multiple definition of auto_index in location blocks");*/
			this->_AutoIndex = parse_auto_index(value);
		}
		else if (key == "cgi_ext")
		{
			/*if (!this->_CgiExt.empty())
				throw runtime_error("Multiple definition of cgi_ext in location blocks");*/
			this->_CgiExt = parse_cgi_ext(value);
		}
		else if (key == "cgi_pass")
		{
			/*if (!this->_CgiPass.empty())
				throw runtime_error("Multiple definition of cgi_pass in location blocks");*/
			this->_CgiPass = parse_cgi_pass(value);
		}
		else if (key == "client_max_body_size")
		{
			if (value.size() != 1)
				throw runtime_error(key + " needs only one arguments");
			this->_HasClientMaxBodySize = true;
			this->_ClientMaxBodySize = parse_body_size(value[0]);
		}
		else if (key == "return")
		{
			if (this->_ReturnCode != -1)
				throw runtime_error("Multiple definition of return in location blocks");
			if (value.empty() || value.size() > 2)
				throw runtime_error(key + " needs one or two arguments");
			this->_HasReturn = true;
			this->_ReturnCode = parser_return_code(value[0], value.size());
			if (value.size() == 2)
				this->_ReturnTarget = value[1];
		}
		else if (key == "upload_store")
		{
			this->_UploadStore = parse_upload_store(value);
			this->_HasUploadStore = true;
		}
		else 
			throw runtime_error(key + " is not a valid instructions in location bloc");
	}
	i++; // saute le "}" vant de rendre le i aparse bloc server
}

