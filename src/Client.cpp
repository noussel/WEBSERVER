# include "../includes/Client.hpp"

int Client::getStatusClient() const
{
    return status_code_client;
}

std::string Client::generateResponse(Connection& conn)
{
    std::string response;
    std::string getMeth;
    getMeth = conn.request.getMethod();
if (getMeth == "GET")
{
    std::string body = "<html><body><h1>GET request for " + conn.request.getPath() + "</h1></body></html>";
    std::stringstream ss;
    ss << "HTTP/1.1 200 OK\r\n"
       << "Content-Type: text/html\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "\r\n"
       << body;

    response = ss.str();
}

    else if (getMeth == "POST")
    {
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: 49\r\n"
                   "\r\n"
                   "<html><body><h1>POST request received</h1></body></html>";
    }
    else if (getMeth == "DELETE")
    {
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: 51\r\n"
                   "\r\n"
                   "<html><body><h1>DELETE request received</h1></body></html>";
    }
    else
    {
        response = "HTTP/1.1 405 Method Not Allowed\r\n"
                   "Content-Length: 0\r\n"
                   "\r\n";
    }
    
    return response;
}