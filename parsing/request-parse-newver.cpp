# include "../includes/request-parse.hpp"

HttpRequestParser::HttpRequestParser()
{
    req.method = "";
    req.path = "";
    req.version = "";
    req.body = "";
    req.headers.clear();
    req.parsed = false;
    req.status = 0;
}

HttpRequestParser::~HttpRequestParser() {}

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

// std::string HttpRequestParser::getHeader(const std::string& key) const
// {
//     std::map<std::string, std::string>::const_iterator it = req.headers.find(key);
//     if(it != req.headers.end())
//     {
//         return it->second;
//     }
//     return "";
// }

std::map<std::string, std::string> HttpRequestParser::getHeader() const
{
    std::map<std::string, std::string> mapHeader;

    std::map<std::string, std::string>::const_iterator it;
    for(it = req.headers.begin(); it != req.headers.end(); it++)
    {
        mapHeader[it->first] = it->second;
    }
    return mapHeader;
}

std::string HttpRequestParser::getValue(std::string key) const
{
    std::map<std::string, std::string> mapHeader = getHeader();
    std::string value = "";
    std::map<std::string, std::string>::const_iterator it = mapHeader.find(key);
    if(it != mapHeader.end())
        value = it->second;
    else
        value = "";
    return value;
}

int HttpRequestParser::getStatus() const
{
    return req.status;
}

std::string HttpRequestParser::getBody() const
{
    return req.body;
}

// bool HttpRequestParser::isParsed() const
// {
//     return true;
// }

void HttpRequestParser::print()
{
    std::cout << "Method: " << req.method << std::endl;
    std::cout << "Path: " << req.path << std::endl;
    std::cout << "Version: " << req.version << std::endl;
    std::cout << "Body: " << req.body << std::endl;
    std::cout << "Header: " << std::endl;
    for(std::map<std::string, std::string>::const_iterator it = req.headers.begin(); it != req.headers.end(); it++)
    {
        std::cout << " " << it->first << " : " << it->second << std::endl;
    }
    std::cout << "Status: " << req.status << std::endl;
}

HttpRequestParser& HttpRequestParser::operator=(const HttpRequestParser& src)
{
    if(this != &src)
    {
        this->req.body = src.req.body;
        this->req.method = src.req.method;
        this->req.path = src.req.path;
        this->req.version = src.req.version;
        this->req.parsed = src.req.parsed;
        this->req.headers = src.req.headers;
    }
    return *this;
}

HttpRequestParser::HttpRequestParser(const HttpRequestParser& src)
{
    *this = src;
}


int isNumeric(const char *bytes)
{
    int i = 0;
    while(bytes[i] && isspace(bytes[i]))
        i++;
    if(bytes[i] == '+')
        i++;
    if(bytes[i] == '-')
        return 0;
    while(bytes[i] && isdigit(bytes[i]))
        i++;
    if(!bytes[i])
        return 1;
    return 0;
}

int toHex(std::string& len)
{
    int sizee;

    
    try{
        sizee = strtol(len.c_str(), NULL, 16);
        return sizee;
    }
    catch(std::exception& e)
    {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}

bool HttpRequestParser::parseRequest(std::string& raw)
{
    //begin of req parse
    size_t EndofHeader = raw.find("\r\n\r\n");
    if(EndofHeader == std::string::npos)
    {
        std::cerr << "Request is incompleted!" << std::endl;
        return false;
    }
    size_t pos = raw.find("\r\n");
    std::string reqLine;
    if(pos != std::string::npos)
    {
        reqLine = raw.substr(0, pos); //extract full first line;
        size_t space1 = reqLine.find(' ');
        if(space1 != std::string::npos)
        {
            req.method = reqLine.substr(0, space1); //store method
        }
        size_t space2 = reqLine.find(' ', space1 + 1);
        if(space2 != std::string::npos)
        {
            req.path = reqLine.substr(space1 + 1, space2 - space1 - 1); //store path
        }
        if(space2 != std::string::npos)
        {
            req.version = reqLine.substr(space2 + 1); //store path
        }
        //finish of extraction req line
    }
    //handle error for request line
    if(!methodIsInvalid())
    {
        req.status = 405;
        std::cout << "405 Method Not Allowed!" << std::endl;
        return req.parsed = true;
    }
    if(!pathIsInvalid())
    {
        //already handled in the fct
        return req.parsed = true;
    }
    if(httpVersionIsInvalid())
    {
        return req.parsed = true;
    }
    
    std::string headerSection = raw.substr(pos + 2, EndofHeader - (pos + 2));
    size_t start = 0;
    std::string key;
    std::string value;
    while(start < headerSection.size())
    {
        size_t LineEnd = headerSection.find("\r\n", start); //end of current line
        size_t len = (LineEnd == std::string::npos) ? headerSection.size() - start : LineEnd - start;
        std::string line = headerSection.substr(start, len); //One header line
        size_t colon = line.find(": ");
        if(colon != std::string::npos)
        {
            key = line.substr(0, colon);
            value = line.substr(colon + 2);
            req.headers[key] = value;
        }
        if(LineEnd == std::string::npos)
            break;
        start = LineEnd + 2; //jump past \r\n to next line
    }
    std::string bytesBody;
    if(EndofHeader != std::string::npos) //endofheaderfound \r\n\r\n
    {
        size_t start_body = EndofHeader + 4;
        for(std::map<std::string, std::string>::iterator it = req.headers.begin(); it != req.headers.end(); it++)
        {
            if(it->first == "Content-Length" || it->first == "content-length" || it->first == "CONTENT-LENGTH")
            {
                bytesBody = it->second;
                if(!isNumeric(bytesBody.c_str())) //check if content lenght is numeric and positive
                    return false;
                if(bytesBody.empty())
                    return (req.parsed = true); //here body is empty
                else
                {
                    req.contentLength = atoi(bytesBody.c_str());
                    if(!headerIsInvalid())
                    {
                        return false;
                    }
                    if(raw.size() >= start_body + req.contentLength)
                    {
                        //full body received, we must store the body,
                        req.body = raw.substr(start_body, req.contentLength);
                        return (req.parsed = true);
                    }
                    else
                        return false; //incomplete, wait for more data
                }
                break;
            }
            //now i have byte from content lenght
        }
    }
    return (req.parsed = true);
}

std::string HttpRequestParser::getPathinfo()
{
    std::string path = getPath();
    
    // Remove query string if present
    size_t queryPos = path.find('?');
    if (queryPos != std::string::npos)
        path = path.substr(0, queryPos);
    
    // Find first dot (assumes CGI script has extension)
    size_t dotPos = path.find('.');
    if (dotPos == std::string::npos)
        return "";
    
    // Find slash after the dot position
    size_t slashPos = path.find('/', dotPos);
    if (slashPos == std::string::npos)
        return "";
    
    return path.substr(slashPos);
}

std::string HttpRequestParser::getQuery() //if method is get so here i will get query that is after ? in path
{
    std::string Query = "";

    if(getMethod() == "GET")
    {
        std::string path = getPath();
        size_t pos = path.find('?');
        if(pos != std::string::npos)
        {
            //i will check on query string
            Query = path.substr(pos + 1);
        }
    }
    return Query;
}

int HttpRequestParser::methodIsInvalid()
{
    if(req.method != "GET" && req.method != "POST" && req.method != "DELETE")
        return 0;
    return 1;
}

bool isPrintableString(const std::string& s)
{
    /*
    Best practice
        Always cast to unsigned char before std::isprint to avoid 
        undefined behavior with negative char.
    */
    for(std::string::const_iterator it = s.begin(); it != s.end(); it++)
    {
        if(!std::isprint(static_cast<unsigned char>(*it)))
            return false;
    }
    return true;
}

int HttpRequestParser::pathIsInvalid()
{
    std::cout << "ssssssssssssss" << std::endl;
    //404 if path not found
    //414 path too long to handle
    //400 if path contain a char not allowed
    if(req.path.empty())
    {
        std::cout << "No path found!" << std::endl;
        req.status = 404;
        return 0;
    }
    if(req.path.size() > 2048)
    {
        std::cout << "Path too long to handle" << std::endl;
        req.status = 414;
        return 0;
    }
    if(!isPrintableString(req.path))
    {
        std::cout << "path contain a character not allowed" << std::endl;
        std::cout << "Bad request" << std::endl;
        req.status = 400;
        return 0;
    }
    return 1;
}

int HttpRequestParser::httpVersionIsInvalid()
{
    std::cout << "sssccccccccccsssssssssss" << std::endl;
    
    if (req.version != "HTTP/1.0" && req.version != "HTTP/1.1")
    {
        std::cout << "ver: " << req.version << std::endl;
        req.status = 505;
        std::cout << "505 HTTP Version Not Supported" << std::endl;
        return 0;
    }
    if(req.version.empty())
    {
        req.status = 400;
        std::cout << "Bad request" << std::endl;
        return 0;
    }
    return 1;
}

bool strToint(std::string value)
{
    std::string str = value;
    std::istringstream ss(str);

    int num = 0;
    return (ss >> num) && ss.eof();
}

int HttpRequestParser::headerIsInvalid()
{
    std::cout << "ssscccccvvvvvvvvvvvvccccccsssssssssss" << std::endl;
    //check on header if body is not empty
    std::map<std::string, std::string>::const_iterator it;
    std::map<std::string, std::string>::const_iterator ct;
    it = req.headers.find("Content-Length");
    
    if(it != req.headers.end())
    {
        req.status = 501;
        std::cout << "Not implemented" << std::endl;
        return 0;
    }
    if(req.method == "GET" || req.method == "DELETE") //GET/DELETE should not have a body (in 42 subject rules)
    {
        if(!req.body.empty() || it != req.headers.end())
        {
            req.status = 400;
            std::cout << "Bad request" << std::endl;
            return 0;
        }
    }
    else if(req.method == "POST")
    {
        if(it == req.headers.end())
        {
            req.status = 400;
            std::cout << "content len missing" << std::endl;
            return 0;
        }
        if(!strToint(getValue("Content-Length")) || req.contentLength < 0)
        {
            req.status = 400;
            std::cout << "Content-Lenght malformeed" << std::endl;
            return 0;
        }
        if(static_cast<size_t>(req.contentLength) > info.client_max_body_size ||
            (!req.contentLength && !req.body.empty())) //Content-Length = 0 but body exists ❌
        {
            req.status = 413;
            std::cout << "Body length bigger than expected" << std::endl;
            return 0;
        }
        ct = req.headers.find("Content-Type");
        if(req.contentLength && ct == req.headers.end())
        {
            req.status = 400;
            std::cout << "Content type missing" << std::endl;
            return 0;
        }
        // if()
    }
        /*check on  -> 
    no body → done
    has body → go read body*/
    if (req.method == "POST")
    {
        if (req.contentLength == 0)
        {
            // no body → done
            return 1;
        }
        else
        {
            // has body → wait until full body received
            return 0;
        }
    }
    return 1;
}
