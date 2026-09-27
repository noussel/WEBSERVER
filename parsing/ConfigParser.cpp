#include "ConfigParser.hpp"
#include "ServerConfig.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>
#include <cstdlib>
#include <map>
#include <set>


static std::string trim(const std::string &s)
{
    size_t start = s.find_first_not_of(" \t");
    size_t end = s.find_last_not_of(" \t");

    if (start == std::string::npos)
        return "";

    return s.substr(start, end - start + 1);
}

static void checkSemicolon(const std::string &line)
{
    size_t count = 0;

    for (size_t i = 0; i < line.size(); i++)
    {
        if (line[i] == ';')
            count++;
    }

    if (count == 0)
        throw std::runtime_error("Missing ';'");

    if (count > 1)
        throw std::runtime_error("Multiple semicolons not allowed");

    if (line[line.size() - 1] != ';')
        throw std::runtime_error("Semicolon must be at end");
}


ConfigParser::ConfigParser() {}

ConfigParser::ConfigParser(const std::string &filename)
{
    _filename = filename;
}

std::vector<ServerConfig> ConfigParser::parse()
{
    std::ifstream file(_filename.c_str());
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file");

    std::string line;

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "server")
        {
            if (!std::getline(file, line))
                throw std::runtime_error("Expected '{' after server");

            line = trim(line);
            if (line != "{")
                throw std::runtime_error("Expected '{' after server");

            parseServerBlock(file);
        }
        else
            throw std::runtime_error("Invalid directive outside server");
    }

    return _servers;
}


void ConfigParser::parseServerBlock(std::ifstream &file)
{
    ServerConfig server;
    std::string line;

    bool listenSet = false;
    bool serverNameSet = false;
    bool indexSet = false;

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "}")
        {
            if (!listenSet)
                throw std::runtime_error("listen missing");
            for(size_t i = 0; i < _servers.size(); i++)
            {
                if(_servers[i].port == server.port &&
                   _servers[i].host == server.host)
                {
                    throw std::runtime_error("Duplicate server block (same host, port)");
                }
            }
            _servers.push_back(server);
            return;
        }

        if (line.find("location") == 0 && line.find('{') != std::string::npos)
        {
            parseLocationBlock(file, server, line);
            continue;
        }

        checkSemicolon(line);
        line.erase(line.size() - 1);

        size_t pos = line.find(' ');
        if (pos == std::string::npos)
            throw std::runtime_error("Invalid directive");

        std::string key = line.substr(0, pos);
        std::string value = trim(line.substr(pos + 1));

        if (key == "listen")
        {
            int port = std::atoi(value.c_str());
        
            if(port <= 0 || port >= 65535)
                throw std::runtime_error("invalid port");
            server.port.push_back(port);
        }
        else if (key == "server_name")
        {
            if (serverNameSet)
                throw std::runtime_error("server_name duplicated");

            server.server_name = value;
            serverNameSet = true;
        }
        else if (key == "index")
        {
            if (indexSet)
                throw std::runtime_error("index duplicated");

            server.index = value;
            indexSet = true;
        }
        else if (key == "client_max_body_size")
        {
            if (value[value.size() - 1] != 'M')
                throw std::runtime_error("Invalid body size");

            int size = std::atoi(value.substr(0, value.size() - 1).c_str());
            server.client_max_body_size = size * 1024 * 1024;
        }
        else if (key == "error_page")
        {
            size_t sp = value.find(' ');
            if (sp == std::string::npos)
                throw std::runtime_error("Invalid error_page");

            int code = std::atoi(value.substr(0, sp).c_str());
            std::string path = trim(value.substr(sp + 1));

            server.error_pages[code] = path;
        }
        else if(key == "host")
        {
            // std::cout << "---------------------->\n";
            server.host = value;
        }
        else
            throw std::runtime_error("Unknown directive: " + key);
    }

    throw std::runtime_error("Missing '}' in server");
}


void ConfigParser::parseLocationBlock(std::ifstream &file, ServerConfig &server, const std::string &firstLine)
{
    LocationConfig location;
    std::string line;

    std::set<std::string> usedKeys;

    size_t start = firstLine.find(' ');
    size_t brace = firstLine.find('{');

    if (start == std::string::npos || brace == std::string::npos)
        throw std::runtime_error("Invalid location syntax");

    location.path = trim(firstLine.substr(start, brace - start));

    while (std::getline(file, line))
    {
        line = trim(line);

        if (line.empty() || line[0] == '#')
            continue;

        if (line == "}")
            break;

        checkSemicolon(line);
        line.erase(line.size() - 1);

        size_t pos = line.find(' ');
        if (pos == std::string::npos)
            throw std::runtime_error("Invalid directive in location");

        std::string key = line.substr(0, pos);
        std::string value = trim(line.substr(pos + 1));

        if (usedKeys.count(key))
            throw std::runtime_error("Duplicate in location: " + key);

        usedKeys.insert(key);

        if (key == "allowed_methods")
        {
            std::stringstream ss(value);
            std::string m;

            while (ss >> m)
                location.allowed_methods.push_back(m);
        }
        else if (key == "autoindex")
        {
            if (value == "on")
                location.autoindex = true;
            else if (value == "off")
                location.autoindex = false;
            else
                throw std::runtime_error("Invalid autoindex");
        }
        else if (key == "upload_path")
            location.upload_path = value;

        else if (key == "root")
            location.root = value;

        else if (key == "cgi_pass")
        {
            location.cgi_pass = value;
            location.cgi_ext[location.path] = value;
        }
        else if(key == "return")
        {
            std::stringstream ss(value);

            int code;
            std::string url;
            ss >> code >> url;

            location.has_return = true;
            location.number = code;
            location.return_url = url;


            std::cout << "------------> " <<  location.has_return << std::endl;
            std::cout << "------------> " <<  location.number << std::endl;
            std::cout << "------------> " <<  location.return_url << std::endl;

        }

        else
            throw std::runtime_error("Unknown directive: " + key);
    }

    server.locations.push_back(location);
}
