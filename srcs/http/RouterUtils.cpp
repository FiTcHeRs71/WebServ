#include "../../includes/Router.hpp"


string findValue(const string &headers, const string &toFind)
{
	size_t idx = headers.find(toFind);
	if (toFind == "name=" && idx != string::npos && idx > 0)
	{
		if (headers[idx - 1] == 'e')
			idx = headers.find(toFind, idx + 5);
	}
	if (idx == string::npos)
		return "";
	string value;
	bool quote = false;
	for(size_t i = idx + toFind.size(); i < headers.size(); i++)
	{
		if (headers[i] == '\"')
		{
			if (quote == true)
				break ;
			quote = true;
			continue ;
		}
		else if ((headers[i] == ' ' || headers[i] == '	' || headers[i] == ';') && quote == false)
			break ;
		value += headers[i];
	}
	return (value);
}

void fillHeaders(const string &headers, TMultipartPart &part)
{
	part.ContentType = findValue(headers, "Content-Type: ");
	part.Filename = findValue(headers, "filename=");
	part.Name = findValue(headers, "name=");
}

int	writeInFile(const string &filename, const string &body, string &written)
{
	string path = filename;
	struct stat st;
	for (size_t i = 1; stat(path.c_str(), &st) == 0; i++)
	{
		if (S_ISREG(st.st_mode))
		{
			path = filename;
			size_t suffix = path.rfind('.');
			size_t slash = path.rfind('/');
			if (suffix == string::npos || slash > suffix)
				suffix = path.size();
			ostringstream oss;
			oss << "_" << i;
			path.insert(suffix, oss.str());
			continue ;
		}
		else
			return (400);
	}
	ofstream	file(path.c_str(), ios::binary);
	if (!file)
		return (500);
	if (body.size() > 0)
		file.write(body.c_str(), body.size());
	written = path;
	return (0);
}

int	sanitizeAndWrite(const LocationConfig &location, string &written, const string &name, const string &data)
{
	string basename = sanitize_filename(name);
	if (basename.empty())
		return -1;
	string path = location.getUploadStore() + "/" + basename;
	int code = writeInFile(path, data, written);
	return (code);
}

Response uploadMultipart(const ServerConfig &server,
						const LocationConfig &location,
						vector<TMultipartPart> parts)
{
	string locName;
	for (size_t i = 0; i < parts.size(); i++)
	{
		string written;
		int code = sanitizeAndWrite(location, written, parts[i].Filename, parts[i].Data);
		if (code < 0)
			continue ;
		else if (code > 0)
			return (Response::BuildError(code, server));
		else if (locName.empty())
		{
			size_t slash = written.rfind('/');
			locName = (slash == string::npos) ? written : written.substr(slash + 1);
		}
	}
	if (locName.empty())
		return (Response::BuildError(400, server));
	Response res;
	res.SetStatus(201);
	res.SetBody("");
	res.SetHeader("Location", location.getPath() + "/" + locName);
	return (res);
}

Response	upload(const Request &request,
						const ServerConfig &server,
						const LocationConfig &location)
{
	string body = request.getBody();
	string written;
	int code = sanitizeAndWrite(location, written, request.getPath(), body);
	if (code < 0)
		return (Response::BuildError(400, server));
	else if (code > 0)
		return (Response::BuildError(code, server));
	size_t slash = written.rfind('/');
	string locName = (slash == string::npos) ? written : written.substr(slash + 1);
	Response res;
	res.SetStatus(201);
	res.SetBody("");
	res.SetHeader("Location", location.getPath() + "/" + locName);
	return (res);
}

bool	findBoundary( const string &value, string &boundary, int idx)
{
	bool quote = false;
	for (size_t i = idx; i < value.size(); i++)
	{
		if (value[i] == '\"')
		{
			if (i == idx)
			{
				quote = true;
				continue ;
			}
			else if (i > idx && quote == true)
			{
				quote = false;
				break ;
			}
			else
				return false;
		}
		else if ((value[i] == ' ' || value[i] == '	' || value[i] == ';') && !quote)
			break ;
		boundary += value[i];
	}
	if (quote == true)
			return false;
	return true;
}