#ifndef LOCATIONCONFIG_HPP
#define LOCATIONCONFIG_HPP

#include <iostream>
#include <vector>
#include <map>
class LocationConfig
{
    public :
        LocationConfig() : autoindex(false) {}

        std::string path;
        std::string root;
        std::vector<std::string> allowed_methods;
        std::string upload_path;
        bool autoindex;
        std::string cgi_pass;
        std::map<std::string, std::string> cgi_ext;// key: extension, value: cgi_path
        bool has_return;    
        int number;
        std::string return_url;

};

#endif
   