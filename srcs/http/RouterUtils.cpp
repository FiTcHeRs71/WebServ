#include "../../includes/Router.hpp"
#include <string>

static bool	isStartParam(const string &s, size_t idx)
{
	if (idx == 0)
		return true;
	unsigned char c = static_cast<unsigned char>(s[idx - 1]);
	return (!isalnum(c));
}

string	findParam(const string &headers, const string &key)
{
	size_t idx = 0;

	while ((idx = headers.find(key)) != string::npos)
	{
		if (!isStartParam(headers, idx))
		{
			idx += key.size();
			continue ;
		}
		idx += key.size();
		if (idx < headers.size() && headers[idx] == '"')
		{
			size_t endQuote = headers.find('"', idx + 1);
			if (endQuote == string::npos)
				return "";
			return (headers.substr(idx + 1, endQuote - idx - 1));
		}
		size_t end = idx;
		while (end < headers.size() && headers[end] != ' '
				&& headers[end] != '\t' && headers[end] != '\r'
				&& headers[end] != ';')
				end++;
		return (headers.substr(idx, end - idx));
	}
	return "";
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

// bool	parse_multipart(const std::string &body, const std::string &boundary,
// 						vector<TMultipartPart> &out)
// {
// 	if (body.empty() || boundary.empty())
// 		return false;
// 	string delimiter = "--" + boundary + "\r\n";
// 	string endDelimiter = "--" + boundary + "--\r\n";
// 	if (body.size() <= delimiter.size())
// 		return false;
// 	if (body.compare(0, delimiter.size(), delimiter))
// 		return false;
// 	size_t i = delimiter.size();
// 	while(i < body.size())
// 	{
// 		TMultipartPart part;
// 		size_t headersEnd = body.find("\r\n\r\n", i);
// 		if (headersEnd == string::npos)
// 			return false;
// 		string headers = body.substr(i, headersEnd - i);
// 		if (headers.empty())
// 			return false;
// 		fillHeaders(headers, part);
// 		i += headers.size() + 4;
// 		size_t dataEnd = body.find("\r\n--" + boundary, i);
// 		if (dataEnd == string::npos)
// 			return false;
// 		string data = body.substr(i, dataEnd - i);
// 		if (data != delimiter && data != endDelimiter)
// 		{
// 			part.Data = data;
// 			i += data.size() + 2;
// 		}
// 		if (!body.compare(i, delimiter.size(), delimiter) && i != delimiter.size())
// 		{
// 			out.push_back(part);
// 			i += delimiter.size();
// 			continue ;
// 		}
// 		else if (!body.compare(i, endDelimiter.size(), endDelimiter))
// 		{
// 			out.push_back(part);
// 			return true;
// 		}
// 		else
// 			return false;
// 	}
// 	return false;
// }