#ifndef SERVERCONFIG_HPP
#define SERVERCONFIG_HPP
#include <iostream>
#include <vector>
#include<string.h>
#include "LocationConfig.hpp"
#include <map>
#include <map>

class ServerConfig
{
public:
    std::vector<int> port;
    std::string server_name;
    std::string index;
    size_t client_max_body_size;
    std::map<int, std::string> error_pages;
    std::vector<LocationConfig> locations;
    std::string host;


};
 //hada exmple kifach atkhdemi bhad fucntion
//  ServerConfig server = servers[0];  servers = _servers de ton ConfigParser hna rh 3eyti server lightkhdmi biha lihowa lewel

// std::string requestPath = "/script.py"; hna tat7eti bach agha tkhedmi 
// LocationConfig* loc = server.getLocationForRequest(requestPath);

// if (loc) {
//     std::cout << "Path demandé: " << requestPath << std::endl;
//     std::cout << "Root: " << loc->root << std::endl;
//     if (!loc->cgi_pass.empty())
//         std::cout << "CGI: " << loc->cgi_pass << std::endl; hna safi loc .cgi fih path ta3ek hahowa creer tat7edi liya exact chemin wn7eto 

//     std::cout << "Méthodes autorisées: ";
//     for (auto &m : loc->allowed_methods)
//         std::cout << m << " ";
//     std::cout << std::endl;
// } else {
//     std::cout << "Aucune location correspondante trouvée !" << std::endl;
// }

#endif
