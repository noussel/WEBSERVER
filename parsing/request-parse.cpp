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
    req.headers_parsed = false;
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

void HttpRequestParser::setBody(const std::string& bodyChunk)
{
    req.body += bodyChunk;  // Append chunk to body
    std::cout << "📥 Body appended: " << bodyChunk.size() << " bytes | Total: " << req.body.size() << " bytes\n";
}

// bool HttpRequestParser::isParsed() const
// {
//     return true;
// }

void HttpRequestParser::print()
{
    std::cout << "--------------------------------------------\n";
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
        this->req.status = src.req.status;
        this->req.contentLength = src.req.contentLength;
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
    req.method.clear();
    req.path.clear();
    req.version.clear();
    req.body.clear();
    req.headers.clear();
    req.parsed = false;
    req.status = 0;
    req.contentLength = 0;

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
            req.path = reqLine.substr(space1 + 1, space2 - space1 - 1);//store path
            size_t endPath = req.path.find('?');
            if(endPath != std::string::npos)
                req.path = req.path.substr(0, endPath);
            std::cout << "Path1: " << req.path << std::endl;
        }
        if(space2 != std::string::npos)
        {
            req.version = reqLine.substr(space2 + 1); //store ver
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
    if(!httpVersionIsInvalid())
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
    if(!headerIsInvalid())
    {
        req.headers_parsed = true;
        return req.parsed = true;
    }

    if(EndofHeader != std::string::npos) //endofheaderfound \r\n\r\n
    {
        // req.start_body = EndofHeader + 4;
        // req.received = raw.size() - req.start_body;
        
        // // Extract Content-Length for later use
        // std::map<std::string, std::string>::iterator it = req.headers.find("Content-Length");
        // if(it != req.headers.end())
        // {
        //     req.contentLength = std::stoul(it->second);
        // }

        // ✅ Headers parsed successfully - STOP HERE
        // Body will be handled chunk-by-chunk via setBody()
        req.status = 200;
        req.headers_parsed = true;
        return (req.parsed = true);
    }
    
    req.status = 200;
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
    if(req.version.empty())
    {
        req.status = 400;
        std::cout << "Bad request" << std::endl;
        return 0;
    }
    if (req.version != "HTTP/1.0" && req.version != "HTTP/1.1")
    {
        req.status = 505;
        std::cout << "505 HTTP Version Not Supported" << std::endl;
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

    std::string transferEncoding = getValue("Transfer-Encoding");
    if(transferEncoding.empty())
        transferEncoding = getValue("Transfer-Encoding");

    if(!transferEncoding.empty())
    {
        req.status = 501;
        std::cout << "Not implemented" << std::endl;
        return 0;
    }

    std::string contentLength = getValue("Content-Length");
    if(contentLength.empty())
        contentLength = getValue("Content-Length");

    if(contentLength.empty())
    {
        req.contentLength = 0;
        if(req.method == "POST")
        {
            req.status = 411;
            std::cout << "Length Required" << std::endl;
            return 0;
        }
        return 1;
    }

    if(!strToint(contentLength))
    {
        req.status = 400;
        std::cout << "Content-Length malformed" << std::endl;
        return 0;
    }

    req.contentLength = atoi(contentLength.c_str());
    if(req.contentLength < 0)
    {
        req.status = 400;
        return 0;
    }

    std::map<std::string, std::string>::const_iterator ct;
    ct = req.headers.find("Content-Type");

    if(req.method == "POST")
    {
        if(!contentLength.empty() && ct == req.headers.end())
        {
            req.status = 400;
            return 0;
        }
        return 1;
    }

    if((req.method == "GET" || req.method == "DELETE") && req.contentLength > 0)
    {
        req.status = 400;
        std::cout << "Body not allowed for this method" << std::endl;
        return 0;
    }

    if(info.client_max_body_size > 0 && static_cast<size_t>(req.contentLength) > info.client_max_body_size)
    {
        req.status = 413;
        std::cout << "Body length bigger than expected" << std::endl;
        return 0;
    }

    return 1;
}

// std::string generatefile(){
//     std::string chaars = "abcdefghijklmnopABCDEFJHIGKLMNOP123456790";
//     std::string filename;
//     for(int i = 0; i < 5; i++){
//         filename += chars[rand() % chars.size()];
//     }
//     return filename.c_str();
// }

// int HttpRequestParser::handleBody(std::string body)
// {
//     if(body.empty())
//     {
//         body = generatefile();
//         req.file = open(body.c_str(), O_CREAT | O_WRONLY, 0644);
//         if(req.file == -1)
//         {
//             req.status = 500;
//             body.clear();
//             return 0;
//         }
//     }

//     req.bytes_read += body.size();
//     if(req.bytes_read > req.contentLength)
//     {
//         req.status = 413;
//         close(req.file);
//         std::remove(body.c_str());
//         body.clear();
//         req.bytes_read = 0;
//         return 0;
//     }

//     if (write(req.file, body.c_str(), body.size()) == -1)
// 	{
// 		req.status = 500;
// 		close(req.file);
// 		std::remove(body.c_str());
// 		body.clear();
//         req.bytes_read = 0;
// 		return 0;
// 	}
//     else if(req.bytes_read == req.contentLength)
//     {
//         req.status = 200;
//         close(req.file);
//     }
//     return 1;
// }