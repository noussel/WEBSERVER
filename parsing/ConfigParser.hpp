#ifndef CONFIGPARSER_HPP
#define CONFIGPARSER_HPP

#include <string>
#include <vector>
#include <fstream>
#include "ServerConfig.hpp"

class ConfigParser
{
private:
    std::string _filename;
    std::vector<ServerConfig> _servers;

    void parseServerBlock(std::ifstream &file);
    void parseLocationBlock(std::ifstream &file, ServerConfig &server, const std::string &firstLine);

public:
    ConfigParser(const std::string &filename);
    std::vector<ServerConfig> parse();
    ConfigParser();
};

#endif
