#include <string>
#include <netinet/in.h>

// Client class to handle all info about the client's connection
class Client
{
    public:
        int soc;                        // Socket to connect to server
        int port;                       // The server port
        std::string serverAddr;         // IPV4 server address
        struct sockaddr_in servConn;    // Server connecion details

    Client(){};                         // Constructor
    ~Client(){};                        // Virtual destructor
};
