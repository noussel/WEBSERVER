# pragma once

#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <cstring>
#include <cerrno>
#include <ctime>
#include "../parsing/ServerConfig.hpp"
#include "../includes/request-parse.hpp"
#include "../includes/Client.hpp"
#include "../Response/Response.hpp"
# include "../parsing/LocationConfig.hpp"

//Struct to hold client connection data
struct Connection
{
    int fd;
    HttpRequestParser request;
    std::string buffer;      // incoming request buffer
    std::string response;    // outgoing response buffer
    size_t bytes_sent;       // track how much of response has been sent
    bool request_parsed;
    std::time_t last_activity;
    bool force_close;
    Connection();
    Connection(int fd);

    bool headers_parsed;
    bool body_done;
    
    size_t body_start;
    size_t content_length;
    size_t received_body;
    /*
    The map is working as:

        Key: client file descriptor

        Value: Connection object containing fd and buffer string
    */
};

class Server
{
    private:
        std::vector<int> _listen_fds;
        std::vector<pollfd> _pfds;
        std::map<int, Connection> connections; // Store client connections
        // Socket info
        int _server_fd;
        int _epoll_fd;
        int _client_fd;
        ServerConfig data; //to get host 127.0.0.1, 
        struct sockaddr_in _address;
    public:
        Server();
        ~Server();

        //Core fuctions
        void initSockets(); //Setup server socket
        void runSockets();  //Main event loop
        // Configuration (implement later)
        void setConfig(const ServerConfig& config) { data = config;}
        // void loadConfig(std::string& path);
        
        //Client handling
        void acceptClient(int server_fd);
        void handleClient(int fd);
        void removeClient(int fd);
        LocationConfig* findLocation(const std::string& path, std::vector<LocationConfig>& locations);
        bool shouldCloseConnection(const Connection& conn);
        // Getters
        int getServerFd() const { return _server_fd; }

        void handleWrite(int fd);
        void updateClientActivity(int fd);
        void closeTimedOutClients();

        void bodyRemainder(Connection &conn);
};

/*
In C++, "poll" typically refers to the poll() system call used for I/O multiplexing, 
allowing a program to efficiently monitor multiple file descriptors (like sockets, pipes, or files)
to see if they are ready for reading, writing, or if an error occurred.
*/