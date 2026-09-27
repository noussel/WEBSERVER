#include "Response.hpp"
#include <sys/stat.h>
#include <fstream>
#include <sstream>
#include <dirent.h>

std::string response::getStatusMessage(int code)
{
    switch (code)
    {
        case 400: return "Bad Request";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 408: return "Request Timeout";
        case 411: return "Length Required";
        case 413: return "Payload Too Large";
        case 415: return "Unsupported Media Type";
        case 500: return "Internal Server Error";
        case 504: return "Gateway Timeout";
        case 505: return "HTTP Version Not Supported";
        default:  return "Unknown Error";
    }
}

std::string response::buildErrorResponse(int code)
{
    std::string body;
    std::string path;
    std::map<int, std::string>::const_iterator it;

    it = serverConfig->error_pages.find(code);

    if (it != serverConfig->error_pages.end())
    {
        path = it->second;
        std::string fullPath;

        if (!path.empty() && path[0] == '.')
            fullPath = path;
        else
            fullPath = loc.root + "/" + path;

        body = readFile(fullPath);
    }

    if (body.empty())
    {
        std::ostringstream oss;
        oss << "<html><head><title>Error " << code << "</title></head>"
            << "<body><h1>" << code << " - " << getStatusMessage(code)
            << "</h1></body></html>";
        body = oss.str();
    }

    std::ostringstream sizeStream;
    std::ostringstream codeStream;

    sizeStream << body.size();
    codeStream << code;

    return "HTTP/1.1 " + codeStream.str() + " " + getStatusMessage(code) + "\r\n"
           "Content-Type: text/html\r\n"
           "Content-Length: " + sizeStream.str() + "\r\n"
           "Connection: close\r\n"
           "\r\n" + body;
}

bool response::isValidPath(const std::string& path)
{
    struct stat st;
    return (stat(path.c_str(), &st) == 0);
}

bool response::isDirectory(const std::string& path)
{
    struct stat st;

    if (stat(path.c_str(), &st) != 0)
        return false;

    return S_ISDIR(st.st_mode);
}

bool response::isReadable(const std::string& path)
{
    return (access(path.c_str(), R_OK) == 0);
}

std::string response::handleDirectory(std::string path)
{
    if (!path.empty() && path[path.size() - 1] != '/')
        path += "/";

    if (!serverConfig->index.empty())
        return path + serverConfig->index;

    return path + "index.html";
}

std::string response::readFile(const std::string& path)
{
    std::ifstream file(path.c_str(), std::ios::in | std::ios::binary);

    if (!file)
        return "";

    char buffer[1024];
    std::string body;

    while (file.read(buffer, sizeof(buffer)))
        body.append(buffer, file.gcount());

    body.append(buffer, file.gcount());

    return body;
}



std::string response::getContentType(const std::string& path)
{
    if (path.find(".html") != std::string::npos)
        return "text/html";
    else if (path.find(".css") != std::string::npos)
        return "text/css";
    else if (path.find(".js") != std::string::npos)
        return "application/javascript";
    else if (path.find(".png") != std::string::npos)
        return "image/png";
    else if (path.find(".jpg") != std::string::npos || path.find(".jpeg") != std::string::npos)
        return "image/jpeg";
    else if (path.find(".gif") != std::string::npos)
        return "image/gif";
    else if (path.find(".txt") != std::string::npos)
        return "text/plain";

    return "application/octet-stream";
}

std::string response::buildResponse(const std::string& body, const std::string& contentType)
{
    std::stringstream ss;
    ss << body.size();

    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: " + contentType + "\r\n"
           "Content-Length: " + ss.str() + "\r\n"
           "\r\n" + body;
}

std::string response::generateAutoIndex(const std::string& path)
{
    DIR* dir = opendir(path.c_str());

    if (!dir)
        throw 403;

    std::string body = "<html><body><h1>Index of " + path + "</h1>";

    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL)
    {
        std::string name = entry->d_name;

        if (name == "." || name == "..")
            continue;

        body += "<a href=\"" + name + "\">" + name + "</a><br>";
    }

    body += "</body></html>";

    closedir(dir);

    return buildResponse(body, "text/html");
}

std::string response::handleGet()
{
    try
    {
        loc = getLocation();

        if (request.getMethod() != "GET")
            throw 405;

        std::string reqPath = request.getPath();
        if (reqPath.find(loc.path) == 0 && loc.path != "/")
            reqPath.erase(0, loc.path.length());

        if (!reqPath.empty() && reqPath[0] == '/')
            reqPath.erase(0, 1);
      
         std::string root = loc.root;

        if (!root.empty() && root[root.size() - 1] != '/')
            root += "/";

        std::string fullPath = root + reqPath;

        std::cout << "-----------------> reqPath " << reqPath << std::endl;
        std::cout << "-----------------> root " << root << std::endl;
        std::cout << "-----------------> fullPath " << fullPath << std::endl;

        if (!isValidPath(fullPath))
            throw 404;

        if (isDirectory(fullPath))
        {
            std::string indexPath = handleDirectory(fullPath);

            if (isValidPath(indexPath))
            {
                fullPath = indexPath;
            }
            else
            {
                if (loc.autoindex)
                    return generateAutoIndex(fullPath);

                throw 403;
            }
        }
    
        if (!isReadable(fullPath))
            throw 403;

        std::string ext = getExtension(reqPath);

        if (!loc.cgi_pass.empty() && ext == loc.path)
            return executeCGI();

        std::string body = readFile(fullPath);
        std::string contentType = getContentType(fullPath);

        return buildResponse(body, contentType);
    }
    catch (int code)
    {
        return buildErrorResponse(code);
    }
}