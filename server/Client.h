#ifndef CLIENT_H
#define CLIENT_H

#include <netinet/in.h>
#include <string.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <unistd.h>
#include "functions.h"
#include "Server.h"

// Class for basic connection details fot the client
class Client
{
    public:
        int soc;                    // Socket for the client
        int msgSoc;                 // Socket to get msgs
        int port;                   // port to listen to the client
        struct sockaddr_in conn;    // Connection detaild for the client
        fd_set openSockets;         // FD set to listen
        fd_set readSockets;         // Socket list for select()
        fd_set exceptSockets;       // Exception socket list
        socklen_t connLen;          // Connection length

        // parse the client command and execute it
        void parse_client_command(std::string buf, Server *serv);

        // Send the GETMSG response to the client
        void send_msgs_to_client(std::string buf, Server *serv);
    
    Client(int port) : port(port)
    {
        connLen = sizeof(conn);
        msgSoc = -1;
        soc = -1;
        openSockets = fd_set();
        readSockets = fd_set();
    };

    ~Client(){};
    
};

#endif