#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 8080

int main()
{
    int sock;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // 1️⃣ Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        std::cerr << "Socket failed\n";
        return 1;
    }

    // 2️⃣ Configure server address
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");

    // 3️⃣ Connect
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
    {
        std::cerr << "Connection failed\n";
        return 1;
    }

    // 4️⃣ Send message
    std::string message = "Hello from C++ client!";
    send(sock, message.c_str(), message.size(), 0);

    // 5️⃣ Read response
    read(sock, buffer, 1024);
    std::cout << "Server says: " << buffer << std::endl;

    // 6️⃣ Close
    close(sock);

    return 0;
}
