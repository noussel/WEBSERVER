# include "../includes/Server.hpp"

#define MAX_EVENTS 50
#define CLIENT_TIMEOUT_SECONDS 10

Server::Server() : data()
{
    
}

Connection::Connection() : fd(-1), buffer(""), response(""), bytes_sent(0), request_parsed(false), last_activity(std::time(NULL)), force_close(false), headers_parsed(false), body_done(false), body_start(0), content_length(0), received_body(0) {}
Connection::Connection(int fd) : fd(fd), buffer(""), response(""), bytes_sent(0), request_parsed(false), last_activity(std::time(NULL)), force_close(false), headers_parsed(false), body_done(false), body_start(0), content_length(0), received_body(0) {}

void Server::initSockets()
{
    _server_fd = socket(AF_INET, SOCK_STREAM, 0); //serverSocket
    if(_server_fd == -1)
    {
        std::cerr << "Socket creation failed " << std::endl;
        exit(EXIT_FAILURE);
    }
    //Set non-blocking
    int flags = fcntl(_server_fd, F_GETFL, 0);
    if(flags == -1)
    {
        perror("fcntl F_GETFL");
        return ;
    }
    // Set the O_NONBLOCK flag
    if (fcntl(_server_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        return ;
    }
    int opt = 1;
    if(setsockopt(_server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)\
    {
        perror("setsockopt failed");
        close(_server_fd);
        exit(EXIT_FAILURE);
    }
    memset(&_address, 0, sizeof(_address));
    _address.sin_family = AF_INET;
    int listen_port = data.port;
    if (listen_port <= 0)
        listen_port = 8080;
    _address.sin_port = htons(listen_port);
    _address.sin_addr.s_addr = INADDR_ANY;

    if(bind(_server_fd, (struct sockaddr*)&_address, sizeof(_address)) < 0)
    {
        perror("bind()");
        close(_server_fd);
        exit(EXIT_FAILURE);
    }

    if(listen(_server_fd, SOMAXCONN) < 0)
    {
        perror("listen()");
        close(_server_fd);
        exit(EXIT_FAILURE);
    }

    _epoll_fd = epoll_create1(0);
    if(_epoll_fd == -1)
    {
        perror("epoll_create1()");
        close(_server_fd);
        exit(EXIT_FAILURE);
    }

    struct epoll_event ev;
    ev.events = EPOLLIN; //Ready to read
    ev.data.fd = _server_fd; //Attach FD for identification
    if(epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _server_fd, &ev) == -1)
    {
        perror("epoll_ctl()");
        close(_server_fd);
        close(_epoll_fd);
        exit(EXIT_FAILURE);
    }
    std::cout << "Server listening on port " << listen_port << std::endl;
}

void Server::runSockets()
{
    struct epoll_event events[MAX_EVENTS];
    while(true)
    {
        int nfds = epoll_wait(_epoll_fd, events, MAX_EVENTS, 1000);
        //1000 
        if (nfds == -1)
        {
            if (errno == EINTR)
                continue; // Interrupted by signal
            perror("epoll_wait()");
            break;
        }
        closeTimedOutClients();
        for(int i = 0; i < nfds; i++)
        {
            if(events[i].data.fd == _server_fd)
            {
                acceptClient(_server_fd);
                continue; //added
            }
            updateClientActivity(events[i].data.fd);

            // else
            // {
            //     handleClient(events[i].data.fd);
            // }
            if(events[i].events & EPOLLIN) {
                handleClient(events[i].data.fd);
            }
            if(events[i].events & EPOLLOUT) {
                handleWrite(events[i].data.fd);
            }
        }
    }
}

/*
Call accept() when server_fd is ready

Set the new client socket to non-blocking using fcntl()

Add the client fd to epoll with EPOLLIN flag

Store client info (create a Connection struct with fd and buffer)
*/
void Server::acceptClient(int server_fd)
{
    struct sockaddr_in client_add;
    socklen_t client_len = sizeof(client_add);

    _client_fd = accept(server_fd, (struct sockaddr *)&client_add, &client_len);
    
    //Set non-blocking
    int flags = fcntl(_client_fd, F_GETFL, 0);
    if(flags == -1)
    {
        perror("fcntl F_GETFL");
        return ;
    }
    // Set the O_NONBLOCK flag
    if (fcntl(_client_fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl F_SETFL O_NONBLOCK");
        return ;
    }
    
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLRDHUP; //EPOLLRDHUP detects client disconnection
    ev.data.fd = _client_fd; //Attach FD for identification
    if(epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, _client_fd, &ev) == -1)
    {
        perror("epoll_ctl()");
        close(_client_fd);
        close(_epoll_fd);
        return ;
    }
    //store client info
    //Create and store connection
    connections [_client_fd] = Connection(_client_fd);//this creates new Connection object
    //here the time of client will be inistilazed
    std::cout << "New client connection detected fd : " << _client_fd << std::endl;
}

/**
 * Implement handleClient()

    Call recv() to read data

    Handle return values:

    0: Data received, append to client's buffer

    = 0: Client disconnected, remove client

    < 0: Error, remove client if fatal

    Check if you have a complete HTTP request (look for "\r\n\r\n")

 */
void Server::handleClient(int fd)
{
    std::vector<char> buffer(4096);
    ssize_t byte_received = recv(fd, buffer.data(), buffer.size(), 0);
    response Response;

    if(byte_received == -1)
    {
        perror("recv failed");
        return ;
    }
    else if(byte_received == 0)
    {
        std::cout << "connection closed fd : " << fd << std::endl;
        removeClient(fd);
        return ;
    }
    
    updateClientActivity(fd);
    
    Connection &conn = connections[fd];
    std::string receivedData(buffer.begin(), buffer.begin() + byte_received);
    conn.buffer += receivedData;
    
    std::cout << "Received " << byte_received << " byte from fd " << fd << std::endl;
    std::cout << "Current buffer size: " << conn.buffer.size() << std::endl;

    // STEP 1: Parse headers if not done yet
    if(!conn.headers_parsed)
    {
        size_t pos = conn.buffer.find("\r\n\r\n");
        if(pos != std::string::npos)
        {
            // Parse the request
            if(!conn.request_parsed)
            {
                if(conn.request.parseRequest(conn.buffer))
                {
                    conn.request_parsed = true;
                    conn.request.print();
                }
            }
            
            std::cout << "✅ Headers parsed\n";
            conn.headers_parsed = true;
            
            // Extract Content-Length from headers
            std::map<std::string, std::string> headers = conn.request.getHeader();
            std::map<std::string, std::string>::iterator it = headers.find("Content-Length");
            if(it != headers.end())
            {
                conn.content_length = atoi(it->second.c_str());
                std::cout << "Content-Length: " << conn.content_length << "\n";
            }
            
            // body_start is right after "\r\n\r\n"
            conn.body_start = pos + 4; // pos + strlen("\r\n\r\n")
            
            std::cout << "Body starts at: " << conn.body_start << "\n";
            
            // If no body expected, mark as done
            if(conn.content_length == 0)
                conn.body_done = true;
        }
        else
            return; // Wait for more data
    }
    
    // STEP 2: Process body in chunks if needed
    if(conn.headers_parsed && !conn.body_done)
    {
        size_t available = conn.buffer.size() - conn.body_start;
        
        if (available > 0)
        {
            size_t to_read = std::min(
                available,
                conn.content_length - conn.received_body
            );
            
            std::string chunk = conn.buffer.substr(conn.body_start, to_read);
            conn.request.setBody(chunk);  // 🔥 Your friend gets the chunk here
            std::cout << chunk << std::endl;
            conn.received_body += to_read;
            conn.body_start += to_read;
            
            std::cout << "📦 Received body chunk: " << to_read << " bytes (total: " << conn.received_body << "/" << conn.content_length << ")\n";
            
            if (conn.received_body >= conn.content_length)
            {
                conn.body_done = true;
                std::cout << "✅ Body fully received\n";
            }
        }
    }
    
    // STEP 3: Generate response when body is complete
    if(conn.headers_parsed && conn.body_done && conn.response.empty())
    {
        Response.setRequest(conn.request);
        
        LocationConfig locationconfig;
        if (!data.locations.empty())
        {
            LocationConfig* loc = findLocation(conn.request.getPath(), data.locations);
            if(loc)
                locationconfig = *loc;
            Response.setLocationConfig(locationconfig);
        }
        Response.setConfig(data);
        
        std::cout << "getter path " << conn.request.getPath() << std::endl;
        conn.response = Response.makeResponse(conn.request.getStatus());
        std::cout << "\033[33m" << conn.response << "\n" << "\033[0m";
        
        conn.bytes_sent = 0;
        struct epoll_event ev;
        ev.events = EPOLLOUT | EPOLLRDHUP;
        ev.data.fd = fd;
        epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
    }
}


void Server::removeClient(int fd)
{
    if(fd < 0)
        return;
    if(connections.find(fd) == connections.end())
        return;
     // Remove from epoll (if using epoll)
    epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL);
    //close socket fd
    close(fd);
    //Remove from map
    connections.erase(fd);
    std::cout << "Client " << fd << " Removed" << std::endl;
}

Server::~Server() {}

LocationConfig* Server::findLocation(const std::string& path, std::vector<LocationConfig>& locations)
{
    // Find the best matching location (longest matching prefix)
    LocationConfig* bestMatch = NULL;
    size_t bestMatchLen = 0;
    
    for (size_t i = 0; i < locations.size(); i++) {
        if (path.find(locations[i].path) == 0) { // Path starts with location path
            if (locations[i].path.length() > bestMatchLen) {
                bestMatchLen = locations[i].path.length();
                bestMatch = &locations[i];
            }
        }
    }
    return bestMatch;
}

bool Server::shouldCloseConnection(const Connection& conn)
{
    // 1. Check HTTP version
    // HTTP/1.0 defaults to close unless Connection: keep-alive
    // HTTP/1.1 defaults to keep-alive unless Connection: close

    /*shouldCloseConnection() is a helper function that determines 
    whether to keep the connection alive or close it after sending the response*/

    std::string verion = conn.request.getVersion();
    std::map<std::string, std::string> header = conn.request.getHeader();
    std::map<std::string, std::string>::const_iterator it = header.find("Connection");

    //if HTTP/1.0
    if(verion == "HTTP/1.0")
    {
        if(it != header.end() && it->second == "keep-alive")
            return false; //keep connection open
        return true; //close connection
    }

    //if HTTP/1.1
    if(verion == "HTTP/1.1")
    {
        if(it != header.end() && it->second == "close")
        {
            std::cout << "am here" << std::endl;
            return true; //close connection
        }
        return false; //keep connection open
    }
    //unkown verion of http - close to be safe
    return true;
}

void Server::handleWrite(int fd) {

    Connection& conn = connections[fd];
    size_t remaining = conn.response.size() - conn.bytes_sent;
    ssize_t sent = send(fd, conn.response.c_str() + conn.bytes_sent, remaining, 0);
    if(sent == -1)
    {
        if(errno != EAGAIN && errno != EWOULDBLOCK)
            removeClient(fd); //error
        return ;// Try again later
    }
    if (sent == 0)
    {
        removeClient(fd);
        return;
    }
    updateClientActivity(fd);
    conn.bytes_sent += static_cast<size_t>(sent);
    if(conn.bytes_sent >= conn.response.size())
    {
        // Response fully sent
        if (conn.force_close || shouldCloseConnection(conn))
        {
            std::cout << "force close " << conn.force_close << std::endl;
            removeClient(fd);
        }
        else
        {
            // Back to reading
            struct epoll_event ev;
            ev.events = EPOLLIN;
            ev.data.fd = fd;
            epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);

            // Clear for next request
            conn.buffer.clear();
            conn.response.clear();
            conn.bytes_sent = 0;
            conn.request_parsed = false;
        }
    }
}

void Server::updateClientActivity(int fd)
{
    std::map<int, Connection>::iterator it = connections.find(fd);
    if(it == connections.end())
        return;
    it->second.last_activity = std::time(NULL);
}

// void Server::closeTimedOutClients()
// {
//     response Response;

//     std::time_t now = std::time(NULL);
//     std::vector<int> timeOutFds;

//     for(std::map<int, Connection>::const_iterator it = connections.begin(); it != connections.end(); it++)
//     {
//         if((now - it->second.last_activity) > CLIENT_TIMEOUT_SECONDS)
//             timeOutFds.push_back(it->first); //fd
//     }
//     for(size_t i = 0; i < timeOutFds.size(); i++)
//     {
//         int fd = timeOutFds[i];
//         std::cout << "Client timeout fd: " << timeOutFds[i] << std::endl;
//         /*
//             // Create a 408 Request Timeout response
//             std::string timeoutResponse = "HTTP/1.1 408 Request Timeout\r\n";
//             timeoutResponse += "Content-Type: text/plain\r\n";
//             timeoutResponse += "Content-Length: 16\r\n";
//             timeoutResponse += "Connection: close\r\n";
//             timeoutResponse += "\r\n";
//             timeoutResponse += "Request Timeout\r\n";
            
//             // Send it to the client
//             send(timeOutFds[i], timeoutResponse.c_str(), timeoutResponse.length(), 0);
//         */
//         //Build response AND store it
//         connections[fd].response = Response.buildErrorResponse(408);
//         connections[fd].bytes_sent = 0;
//         connections[fd].force_close = true;
//         //Switch socket to WRITE mode
//         struct epoll_event ev;
//         ev.events = EPOLLOUT | EPOLLRDHUP;
//         ev.data.fd = fd;
//         epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, &ev);
//         // removeClient(timeOutFds[i]);
//     }
// }

//simple approach
void Server::closeTimedOutClients()
{
    response Response;
    Response.setConfig(data);
    std::time_t now = std::time(NULL);
    std::vector<int> timeOutFds;

    // First, collect all timed out clients
    for(std::map<int, Connection>::iterator it = connections.begin(); it != connections.end(); it++)
    {
        if((now - it->second.last_activity) > CLIENT_TIMEOUT_SECONDS)
        {
            timeOutFds.push_back(it->first);
        }
    }
    
    // Then remove them (don't try to send response)
    for(size_t i = 0; i < timeOutFds.size(); i++)
    {
        int fd = timeOutFds[i];
        // Queue 408 response
        connections[fd].response = Response.buildErrorResponse(408);
        connections[fd].bytes_sent = 0;
        connections[fd].force_close = true;
        
        // Switch to write mode
        handleWrite(fd);
        
        std::cout << "Client timeout fd: " << timeOutFds[i] << std::endl;
        // removeClient(timeOutFds[i]);
    }
}

// void Server::closeTimedOutClients()
// {
//     response Response;
//     std::time_t now = std::time(NULL);
//     std::vector<int> timeOutFds;

//     for(std::map<int, Connection>::const_iterator it = connections.begin(); it != connections.end(); it++)
//     {
//         if((now - it->second.last_activity) > CLIENT_TIMEOUT_SECONDS)
//             timeOutFds.push_back(it->first);
//     }
    
//     // Process each timeout client separately
//     for(size_t i = 0; i < timeOutFds.size(); i++)
//     {
//         int fd = timeOutFds[i];
        
//         // Check if connection still exists (might have been removed)
//         std::map<int, Connection>::iterator it = connections.find(fd);
//         if(it == connections.end())
//             continue; //if i remove it i got seg
        
//         // // Check if socket is still valid
//         // int flag = fcntl(fd, F_GETFD);
//         // if(flag == -1)
//         // {
//         //     removeClient(fd);
//         //     continue;
//         // }
        
//         // Queue 408 response
//         it->second.response = Response.buildErrorResponse(408);
//         it->second.bytes_sent = 0;
//         it->second.force_close = true;
        
//         // Switch to write mode
//         handleWrite(fd);
        
//         std::cout << "Timeout: queued 408 for fd " << fd << std::endl;
//     }
// }


/*
Option 2: Let Response handle location matching internally
Modify Response to accept the full ConfigParser data and find the right location itself.

For your multiplexing code specifically:
Your handleClient() is doing too much. It should:

Read data

Parse when complete

Then pass request + config to Response handler

The Response handler should be responsible for:

Finding the correct location based on request path

Building the response using that location's config

Returning the HTTP response string


*/