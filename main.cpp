#include <iostream>
#include <fstream>
#include <string>
#include "parsing/ConfigParser.hpp"
#include "includes/Server.hpp"

bool hasConfExtension(const std::string &filename)
{
    if (filename.length() < 5)
        return false;
    return filename.substr(filename.length() - 5) == ".conf";
}

int main(int argc, char **argv)
{
    Server server;

    if (argc != 2)
    {
        std::cerr << "Usage: ./webserv <config_file.conf>" << std::endl;
        return 1;
    }

    try
    {
        int nbserver;
        ConfigParser parser(argv[1]);

        // 🔴 SI ERREUR DANS PARSING → THROW → CATCH → PROGRAMME STOP
        std::vector<ServerConfig> servers = parser.parse();

        std::cout << "Nombre de serveurs chargés: "
                  << servers.size() << std::endl;

        nbserver = servers.size();

        for (int i = 0; i < nbserver; i++)
        {
            server.setConfig(servers[i]);
            server.initSockets();
            server.runSockets();
        }
    }
    catch (const std::exception &e)
    {
        // 🔴 ICI : arrêt immédiat si erreur parsing ou runtime
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
