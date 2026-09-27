#include "Response.hpp"

response::response() : serverConfig(NULL) {} //constractor

void response::setRequest(const HttpRequestParser &reqParser) {
    request = reqParser;
}

void response::setLocationConfig(const LocationConfig &loc) {
     this->loc = loc; 
}

void response::setConfig(const ServerConfig &config)
{
    serverConfig = &config; //new method
}


LocationConfig response::getLocation() const
{
    std::string path = request.getPath();
    std::string ext = getExtension(path);

    for (size_t i = 0; i < serverConfig->locations.size(); i++)
    {
        if (serverConfig->locations[i].path == ext)
            return serverConfig->locations[i];
    }

    const LocationConfig* best = NULL;
    size_t bestLen = 0;

    for (size_t i = 0; i < serverConfig->locations.size(); i++)
    {
        const std::string& locPath = serverConfig->locations[i].path;

        if (path.find(locPath) == 0 && locPath.length() > bestLen)
        {
            best = &serverConfig->locations[i];
            bestLen = locPath.length();
        }
    }

    if (best)
        return *best;

    throw 404;
}

HttpRequestParser response::getRequest()const {
    return request;
}

// std::string response::handlePOST() {
//     return error(501);
// }

std::string response::makeResponse(int status) {

    if(status == 200)
    {
        std::string method = request.getMethod();

        if (method == "GET")
            return handleGet();
        else if (method == "DELETE")
            return handleDelete();
        if (method == "POST")
            return handlePOST();
    }
    return buildErrorResponse(status);
}

