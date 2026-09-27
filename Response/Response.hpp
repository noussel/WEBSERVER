#pragma once
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <sys/stat.h>
#include <algorithm> 
#include "../includes/request-parse.hpp"
#include "LocationConfig.hpp"
#include "ServerConfig.hpp"
#include <unistd.h>   // pipe, fork, dup2, execve, read, write
#include <sys/wait.h> // waitpid
#include <sys/time.h> //gettimeofday
#include <fcntl.h>
#include <cstdlib>  

class response {
private:
    HttpRequestParser request;
    const ServerConfig* serverConfig; 
    LocationConfig loc;

    std::string errorhandling();
    std::string executeCGI();
    
    std::string handlePOST();
    std::string handleGet();
    std::string handleDelete();

public:
    response();

    void setRequest(const HttpRequestParser &reqParser);
    void setLocationConfig(const LocationConfig &loc);
    void setConfig(const ServerConfig &serverconfig); 
    
    LocationConfig getLocation()const;
    HttpRequestParser getRequest()const;
    std::string makeResponse(int status);
    std::string buildErrorResponse(int code);
    std::string getStatusMessage(int code);
    std::string saveToFile(HttpRequestParser &request, LocationConfig loc) ;

    std::string buildFullPath();
    std::string generateAutoIndex(const std::string& path);

    bool isValidPath(const std::string& path);
    bool isDirectory(const std::string& path);
    std::string handleDirectory(std::string path);
    bool isReadable(const std::string& path);

    std::string readFile(const std::string& path);
    std::string getContentType(const std::string& path);
    std::string buildResponse(const std::string& body, const std::string& contentType);

};

std::string error(int code);
std::string getExtension(const std::string path);
std::string generatefile();