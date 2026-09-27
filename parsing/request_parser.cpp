# include "../includes/request-parse.hpp"

# include <sstream>
# include <algorithm>
# include <cctype>

namespace
{
	static const size_t MAX_URI_LEN = 2048;
	static const size_t MAX_HEADER_LINE_LEN = 8192;
	static const size_t MAX_HEADER_SECTION_LEN = 16384;
	static const size_t MAX_HEADERS_COUNT = 100;

	std::string trim(const std::string &value)
	{
		size_t start = 0;
		while (start < value.size() && std::isspace(static_cast<unsigned char>(value[start])))
			start++;
		size_t end = value.size();
		while (end > start && std::isspace(static_cast<unsigned char>(value[end - 1])))
			end--;
		return value.substr(start, end - start);
	}

	std::string toLower(const std::string &value)
	{
		std::string out = value;
		std::transform(out.begin(), out.end(), out.begin(), ::tolower);
		return out;
	}

	bool isToken(const std::string &value)
	{
		if (value.empty())
			return false;
		for (size_t i = 0; i < value.size(); i++)
		{
			unsigned char c = static_cast<unsigned char>(value[i]);
			if (!std::isalnum(c) && c != '-' && c != '_')
				return false;
		}
		return true;
	}

	bool isNumeric(const std::string &value)
	{
		if (value.empty())
			return false;
		for (size_t i = 0; i < value.size(); i++)
		{
			if (!std::isdigit(static_cast<unsigned char>(value[i])))
				return false;
		}
		return true;
	}

	bool isSupportedMethod(const std::string &method)
	{
		return method == "GET" || method == "POST" || method == "DELETE";
	}

	bool isValidHttpVersionSyntax(const std::string &version)
	{
		if (version.size() != 8)
			return false;
		if (version.substr(0, 5) != "HTTP/")
			return false;
		if (!std::isdigit(static_cast<unsigned char>(version[5])))
			return false;
		if (version[6] != '.')
			return false;
		if (!std::isdigit(static_cast<unsigned char>(version[7])))
			return false;
		return true;
	}

	bool isSupportedHttpVersion(const std::string &version)
	{
		return version == "HTTP/1.1" || version == "HTTP/1.0";
	}

	std::string canonicalHeaderKey(const std::string &key)
	{
		std::string lower = toLower(key);
		if (lower == "content-length")
			return "Content-Length";
		if (lower == "transfer-encoding")
			return "Transfer-Encoding";
		if (lower == "host")
			return "Host";
		if (lower == "connection")
			return "Connection";
		if (lower == "content-type")
			return "Content-Type";
		return key;
	}
}

HttpRequestParser::HttpRequestParser()
{
	req.method = "";
	req.path = "";
	req.version = "";
	req.body = "";
	req.headers.clear();
	req.status = 200;
	req.parsed = false;
}

HttpRequestParser::~HttpRequestParser() {}

HttpRequestParser& HttpRequestParser::operator=(const HttpRequestParser& src)
{
	if (this != &src)
	{
		req.method = src.req.method;
		req.path = src.req.path;
		req.version = src.req.version;
		req.headers = src.req.headers;
		req.body = src.req.body;
		req.status = src.req.status;
		req.parsed = src.req.parsed;
	}
	return *this;
}

HttpRequestParser::HttpRequestParser(const HttpRequestParser& src)
{
	*this = src;
}

std::string HttpRequestParser::getMethod() const
{
	return req.method;
}

std::string HttpRequestParser::getPath() const
{
	return req.path;
}

std::string HttpRequestParser::getVersion() const
{
	return req.version;
}

std::map<std::string, std::string> HttpRequestParser::getHeader() const
{
	return req.headers;
}

std::string HttpRequestParser::getValue(std::string key) const
{
	std::map<std::string, std::string>::const_iterator it = req.headers.find(key);
	if (it != req.headers.end())
		return it->second;

	std::string lower = toLower(key);
	for (std::map<std::string, std::string>::const_iterator h = req.headers.begin(); h != req.headers.end(); ++h)
	{
		if (toLower(h->first) == lower)
			return h->second;
	}
	return "";
}

std::string HttpRequestParser::getBody() const
{
	return req.body;
}

int HttpRequestParser::getStatus() const
{
	return req.status;
}

void HttpRequestParser::print()
{
	std::cout << "Method: " << req.method << std::endl;
	std::cout << "Path: " << req.path << std::endl;
	std::cout << "Version: " << req.version << std::endl;
	std::cout << "Body: " << req.body << std::endl;
	std::cout << "Header: " << std::endl;
	for (std::map<std::string, std::string>::const_iterator it = req.headers.begin(); it != req.headers.end(); ++it)
		std::cout << " " << it->first << " : " << it->second << std::endl;
}

bool HttpRequestParser::parseRequest(std::string& raw)
{
	req.method.clear();
	req.path.clear();
	req.version.clear();
	req.headers.clear();
	req.body.clear();
	req.status = 0;
	req.parsed = false;

	size_t endOfHeader = raw.find("\r\n\r\n");
	if (endOfHeader == std::string::npos)
		return false;

	if (endOfHeader > MAX_HEADER_SECTION_LEN)
	{
		req.status = 431;
		req.parsed = true;
		return true;
	}

	size_t requestLineEnd = raw.find("\r\n");
	if (requestLineEnd == std::string::npos)
	{
		req.status = 400;
		req.parsed = true;
		return true;
	}

	std::string requestLine = raw.substr(0, requestLineEnd);
	if (requestLine.empty())
	{
		req.status = 400;
		req.parsed = true;
		return true;
	}

	std::istringstream reqLineStream(requestLine);
	std::string extra;
	reqLineStream >> req.method >> req.path >> req.version;
	if (req.method.empty() || req.path.empty() || req.version.empty() || (reqLineStream >> extra))
	{
		req.status = 400;
		req.parsed = true;
		return true;
	}

	if (!isSupportedMethod(req.method))
	{
		req.status = 405;
		req.parsed = true;
		return true;
	}

	if (req.path[0] != '/')
	{
		req.status = 400;
		req.parsed = true;
		return true;
	}

	if (req.path.size() > MAX_URI_LEN)
	{
		req.status = 414;
		req.parsed = true;
		return true;
	}

	if (!isValidHttpVersionSyntax(req.version))
	{
		req.status = 400;
		req.parsed = true;
		return true;
	}

	if (!isSupportedHttpVersion(req.version))
	{
		req.status = 505;
		req.parsed = true;
		return true;
	}

	std::string headerSection = raw.substr(requestLineEnd + 2, endOfHeader - (requestLineEnd + 2));
	size_t start = 0;
	size_t headersCount = 0;
	while (start < headerSection.size())
	{
		size_t lineEnd = headerSection.find("\r\n", start);
		if (lineEnd == std::string::npos)
			lineEnd = headerSection.size();

		std::string line = headerSection.substr(start, lineEnd - start);
		if (line.size() > MAX_HEADER_LINE_LEN)
		{
			req.status = 431;
			req.parsed = true;
			return true;
		}

		size_t colon = line.find(':');
		if (colon == std::string::npos || colon == 0)
		{
			req.status = 400;
			req.parsed = true;
			return true;
		}

		std::string key = trim(line.substr(0, colon));
		std::string value = trim(line.substr(colon + 1));
		if (!isToken(key))
		{
			req.status = 400;
			req.parsed = true;
			return true;
		}

		key = canonicalHeaderKey(key);

		if (key == "Content-Length" && req.headers.find("Content-Length") != req.headers.end() && req.headers["Content-Length"] != value)
		{
			req.status = 400;
			req.parsed = true;
			return true;
		}

		req.headers[key] = value;
		headersCount++;
		if (headersCount > MAX_HEADERS_COUNT)
		{
			req.status = 431;
			req.parsed = true;
			return true;
		}

		start = lineEnd + 2;
	}

	if (req.headers.find("Host") == req.headers.end())
	{
		req.status = 400;
		req.parsed = true;
		return true;
	}

	std::string transferEncoding = getValue("Transfer-Encoding");
	std::string contentLength = getValue("Content-Length");
	if (!transferEncoding.empty() && !contentLength.empty())
	{
		req.status = 400;
		req.parsed = true;
		return true;
	}

	size_t bodyStart = endOfHeader + 4;
	if (!contentLength.empty())
	{
		if (!isNumeric(contentLength))
		{
			req.status = 400;
			req.parsed = true;
			return true;
		}

		size_t bodyLen = static_cast<size_t>(std::strtoul(contentLength.c_str(), NULL, 10));
		if (raw.size() < bodyStart + bodyLen)
			return false;

		if (req.method == "GET" && bodyLen > 0)
		{
			req.status = 400;
			req.parsed = true;
			return true;
		}

		req.body = raw.substr(bodyStart, bodyLen);
		req.status = 200;
		req.parsed = true;
		return true;
	}

	if (!transferEncoding.empty())
	{
		if (req.method == "GET")
		{
			req.status = 400;
			req.parsed = true;
			return true;
		}

		if (toLower(trim(transferEncoding)) == "chunked")
		{
			req.status = 411;
			req.parsed = true;
			return true;
		}
		req.status = 400;
		req.parsed = true;
		return true;
	}

	if (req.method == "POST")
	{
		if (raw.size() > bodyStart)
		{
			req.status = 411;
			req.parsed = true;
			return true;
		}
	}

	if (req.method == "GET" && raw.size() > bodyStart)
	{
		req.status = 400;
		req.parsed = true;
		return true;
	}

	req.status = 200;
	req.parsed = true;
	return true;
}

std::string HttpRequestParser::getPathinfo()
{
	std::string path = getPath();
	size_t queryPos = path.find('?');
	if (queryPos != std::string::npos)
		path = path.substr(0, queryPos);

	size_t dotPos = path.find('.');
	if (dotPos == std::string::npos)
		return "";

	size_t slashPos = path.find('/', dotPos);
	if (slashPos == std::string::npos)
		return "";

	return path.substr(slashPos);
}

std::string HttpRequestParser::getQuery()
{
	std::string query = "";
	if (getMethod() == "GET")
	{
		std::string path = getPath();
		size_t pos = path.find('?');
		if (pos != std::string::npos)
			query = path.substr(pos + 1);
	}
	return query;
}