#ifndef SERVER_H
#define SERVER_H

#include <string>
#include <unordered_map>
#include <list>
#include <netinet/in.h>
#include <string.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <iostream>
#include "ip.h"
#include <vector>
#include <sstream>
#include <time.h>
#include "functions.h"
#include <tuple>

#define MAX_CLIENTS 5           // Maximum number of clients connected
#define MIN_CONN 2              // Minimum number of connections

// Server connection details
struct ServerInfo
{
    bool inUse = false;                     // Socket being used or not
    int msgSoc = -1;                        // Socket to get msgs
    int port = 0;                           // The host port
    std::string ip;                         // Server ip
    std::string id;                         // Server ID
    std::string serverAddr;                 // IPV4 server address
    struct sockaddr_in servConn = sockaddr_in();            // Server connecion details
    time_t lastContact = time(NULL);        // Time since last sent msg
    int contactTry = 0;                     // Number of tries to contact the server
};

// Connection details for each server, message buffer etc...
class Server
{
    private:
        // Connecto to another server
        bool connect_to_server(int sockSlot);

        // Initialize a new socket
        int create_socket();

        // Setup outbound socket
        void setup_serv_conn(int sockSlot);

    public:
        struct ServerInfo servers[5];               // Server list
        int soc;                                    // Socket for the client
        int port;                                   // port to listen to the client
        std::unordered_map<std::string,
            std::vector<std::string>> messages;     // Message map, mapped on group id
        std::string fullError;                      // Error msg forfull server
        struct sockaddr_in conn;                    // Connection detaild for the client
        fd_set openSockets;                         // FD set to listen
        fd_set readSockets;                         // Socket list for select()
        socklen_t connLen;                          // Connection length
        std::string ip;                             // Local ip
        std::string groupName;                      // Group name
        int keepaliveTimeout;                       // time between keepalive msgs
        int maxNumberOfConnectionTries;             // Maximum number of times to try to finalize connection
        int numConnections;                         // Number of active connections
        int minServersCheckTimeout;                 // Timeout for checking if it is needed to connect to another server
        time_t lastKeepalive;                       // Last time a keepalive was sent
        time_t inactiveTimeout;                     // Maximum time for non-contact between servers
        time_t lastMinServerCheck;                  // Time since min server qty was checked
        
        // Servers way to connect to another server
        bool setup_outbound_connection(std::string address, int port);  // Setup outbound connections   

        // Close a connection and reset the ServerInfo struct
        void close_connection(int sockSlot);

        // setup a new incoming connection
        // Return false if slots are full
        bool accept_incoming();

        // Parse the server command
        void parse_server_command(std::string, int sockSlot);

        // Send the SERVERS respnse
        void send_listservers_resp(int sockSlot);

        // Send messages with error handling
        void send_msg(std::string buf, int sockSlot);

        // Recv messages with wait time to finalize setting up the connection
        void set_host_details(std::string buf, int sockSlot);

        // Create the server list in the correct format
        std::string create_server_list();

        // Automatically send keepalive messages to all connected servers
        // Simpler to check a simngle timestamp instead of multiple
        void keepalive();

        /// Remove start and end strings from msg
        std::string rm_msg_tokens(std::string buf);

        // Handle the LEAVE command
        void handle_leave(std::string buf);

        // Send the LEAVE message
        void send_leave(std::string buf);

        // Send messages to other hosts
        void send_msg_command(std::string buf, int sockSlot);

        // Format messages for the SEND_MSG
        std::string format_send_msg(std::string fromGroup, std::string toGroup, std::string msg);

        // Send the GET_MSG command to a specific server
        void send_get_msg(std::string groupId, int sockSlot);

        // Send STATUSRESP to requestee
        void send_status_resp(std::string buf, int sockSlot);

        // Write the message into the list of messages
        void handle_send_msg(std::string buf);

        // Handle the STATUSREQ msg
        void handle_status_req(std::string, int sockSlot);

        // Get server ip and port and set it in the serverInfo
        void get_conn_details();

        // Broadcast a msg to all connected hosts
        void bc_msg(std::string buf);

        // Try to automatically connect to another host
        void auto_connect_to_host(std::string buf);

    Server(int port) : port(port)
    {
        connLen = sizeof(conn);
        soc = -1;
        fullError = "Server full! no slots available";
        ip = get_ip();
        groupName = "P3_GROUP_72";
        openSockets = fd_set();
        readSockets = fd_set();
        keepaliveTimeout = 70;
        inactiveTimeout = 500;
        maxNumberOfConnectionTries = 5;
        lastKeepalive = time(NULL);
        numConnections = 0;
        minServersCheckTimeout = 10;
        lastMinServerCheck = time(NULL);
    };

    ~Server(){};
};

#endif
