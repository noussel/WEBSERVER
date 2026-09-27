#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main(){
    int server_fd;
    int client_fd;
    struct sockaddr_in address;
    socklen_t addr_len = sizeof(address);
    char buffer[1024] = {0};

    //1 ----> creating a socket 
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd < 0){
        std::cerr << "socket failed\n";
        return 1;
    }
    
    // 2 -------> configure address 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // 3 -------> lier (bind)
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0)// ???????//
    {
        std::cerr << "Bind failed\n";
        return 1;
    }

    // 4 ---------> listen
    if (listen(server_fd, 5) < 0)
    {
        std::cerr << "Listen failed\n";
        return 1;
    }
    std::cout << "Server listening on port " << PORT << "...\n";

    // 5 --------> accept
    client_fd = accept(server_fd, (struct sockaddr*)&address, &addr_len);
    if (client_fd < 0)
    {
        std::cerr << "Accept failed\n";
        return 1;
    }

    std::cout << "Client connected!\n";

    // 6️⃣ Read from client
    read(client_fd, buffer, 1024);
    std::cout << "Client says: " << buffer << std::endl;

    // 7️⃣ Send response
    std::string message = "Hello from C++ server!";
    send(client_fd, message.c_str(), message.size(), 0);

    // 8️⃣ Close sockets
    close(client_fd);
    close(server_fd);

    return 0;
}












