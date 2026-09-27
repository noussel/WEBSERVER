# pragma once

# include <iostream>
# include <string>
# include <map>
# include <cstdlib>
# include <sstream>
# include <fcntl.h>
# include <unistd.h>
# include <cstdio>
# include "../parsing/ServerConfig.hpp"

struct HttpRequest
{
    std::string method; //post, get, delete
    std::string path; //upload
    std::string version; // HTTP/1.1
    std::map<std::string, std::string> headers;
    std::string body;
    int status;
    int contentLength;
    bool parsed;
    size_t start_body;
    size_t bytes_len;
    size_t received;
    bool body_complete;
    bool headers_parsed;
};

class HttpRequestParser
{
    private:
        HttpRequest req;
    public:
        HttpRequestParser();
        ~HttpRequestParser();
        HttpRequestParser& operator=(const HttpRequestParser& src);
        HttpRequestParser(const HttpRequestParser& src);
        bool parseRequest(std::string& raw);

        //Getters
        std::string getMethod() const;
        std::string getPath() const;
        std::string getVersion() const;
        std::map<std::string, std::string> getHeader() const;
        std::string getBody() const;
        void setBody(const std::string& bodyChunk);  // Append body chunks
        int getStatus() const;
        // bool isParsed() const;
        std::string getValue(std::string key) const;
        //Debug
        void print();

        size_t getFirstBody();
        std::string getPathinfo();
        std::string getQuery();
        size_t getReceivedBody();

        //coocki
        // std::string getCookie() const;
        
        //handle errors for request
        int methodIsInvalid();
        int pathIsInvalid();
        int httpVersionIsInvalid();
        int headerIsInvalid();
        int handleBody(std::string body);
        ServerConfig info;
};

std::string generatefile();
