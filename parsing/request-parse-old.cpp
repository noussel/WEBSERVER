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
    // std::string chunked;
    // std::string chunked_len;
    // std::string decodeBody;
    // int chunked_size;
    int contentLength;
    if(EndofHeader != std::string::npos) //endofheaderfound \r\n\r\n
    {
        size_t start_body = EndofHeader + 4;
        for(std::map<std::string, std::string>::iterator it = req.headers.begin(); it != req.headers.end(); it++)
        {
            // if(it->first == "Transfer-Encoding")
            // {
            //     chunked = it->second;
            //     break;
            // }
            // else 
            if(it->first == "Content-Length" || it->first == "content-length" || it->first == "CONTENT-LENGTH")
            {
                bytesBody = it->second;
                if(!isNumeric(bytesBody.c_str())) //check if content lenght is numeric and positive
                    return false;
                if(bytesBody.empty())
                    return (req.parsed = true); //here body is empty
                else
                {
                    contentLength = atoi(bytesBody.c_str());
                    if(raw.size() >= start_body + contentLength)
                    {
                        //full body received, we must store the body,
                        req.body = raw.substr(start_body, contentLength);
                        // if(req.method == "POST" && req.body.empty() && contentLength > 0)
                        // {
                        //     throw 400;
                        // }
                        // if(req.method == "GET" && !req.body.empty())
                        // {
                        //     throw 400;
                        // }
                        return (req.parsed = true);
                    }
                    else
                        return false; //incomplete, wait for more data
                }
                break;
            }
            //now i have byte from content lenght
        }
        // if(chunked == "chunked")
        // {
        //     size_t body_pos = start_body;
        //     while(body_pos < raw.size())
        //     {
        //         //fin end of chunk size line \r\n7
        //         size_t endofChunk = raw.find("\r\n", body_pos);
        //         if(endofChunk == std::string::npos)
        //             return false; //Incomplete chunk size
        //         //Extract chunk size in hex
        //         chunked_len = raw.substr(body_pos, endofChunk - body_pos);
        //         if(chunked_len.empty())
        //             return false; //Invalid chunk size
        //         //convert hex to decimal
        //         chunked_size = toHex(chunked_len);
        //         if(chunked_size == 0)
        //         {
        //             if(endofChunk + 4 > raw.size())
        //                 return false; //incomplete final chunk terminator
        //             if(raw.substr(endofChunk + 2, 2) != "\r\n")
        //                 return false; //invalid req
        //             body_pos = endofChunk + 2; //skip final \r\n
        //             break; //end of chunks
        //         }
        //         else if(chunked_size == -1)
        //             return false; //stoi failed
        //         //Move pasr the chunk size line
        //         body_pos = endofChunk + 2;

        //         //check if we have enough data fir this chunk 
        //         if(body_pos + chunked_size + 2 > raw.size())
        //             return false; //Incomplete chunk data
        //         //Extract chunk data
        //         decodeBody += raw.substr(body_pos, chunked_size);
        //         //check two bytes after extracting chunked data
        //         if(raw[body_pos + chunked_size] != '\r' || raw[body_pos + chunked_size + 1] != '\n')
        //             return false; //invalid req
        //         //Move past chunk data and \r\n
        //         body_pos += chunked_size + 2;
        //     }
        // }
    }
    // req.body = decodeBody;
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




    bool parseRequest(std::string& raw) {
        // 1. Find end of headers
        if (EndofHeader == npos) return false;  // Incomplete
        
        // 2. Parse request line
        if (method invalid) {
            req.status = 405;
            req.parsed = true;
            return true;  // Complete, but error
        }
        
        // 3. Parse headers
        // ...
        
        // 4. Parse body
        if (Content-Length missing for POST) {
            req.status = 411;
            req.parsed = true;
            return true;  // Complete, but error
        }
        
        // 5. Body too large
        if (contentLength > max_size) {
            req.status = 413;
            req.parsed = true;
            return true;  // Complete, but error
        }
        
        // 6. Success!
        req.status = 0;
        req.parsed = true;
        return true;
    }
