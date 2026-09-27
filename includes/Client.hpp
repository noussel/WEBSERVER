# pragma once

# include "../includes/Server.hpp"
# include <fstream>
# include <sstream>

struct Connection;

class Client
{
    private:
        int status_code_client;
    public:
        std::string generateResponse(Connection &conn);
        int getStatusClient() const;
};