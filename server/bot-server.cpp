#include "bot-server.h"

using namespace std;

void client_loop(Client *cli, char *buf, int bufSize, int *maxfds, Server *serv, timeval *tv)
{
    cli->readSockets = fd_set(cli->openSockets);
    memset(buf, 0, bufSize);
    int n = select(*maxfds + 1, &cli->readSockets, NULL, NULL, tv);
    
    if(n < 0)
    {
        perror("select failed ");
        exit(-1);
    }
    else
    {
        if(FD_ISSET(cli->soc, &cli->readSockets))
        {
            if((cli->msgSoc = accept(cli->soc, (struct sockaddr *)&cli->conn, &cli->connLen)) > 0)
            {
                cout << "Client connected! " << endl;
            }
            else
            {
                cout << "Failed to connect! " << endl;
            }
        }

        
        FD_SET(cli->msgSoc, &cli->openSockets);
        *maxfds = max(*maxfds, cli->msgSoc);

        while(n-- > 0)
        {
            // Check if message is waiting and read
            if(FD_ISSET(cli->msgSoc, &cli->readSockets))
            {
                if(read(cli->msgSoc, buf, bufSize) == 0)
                {
                    // close connection to client
                    cout << "Client disconnected" << endl;
                    close(cli->msgSoc);
                    FD_CLR(cli->msgSoc, &cli->openSockets);
                    cli->msgSoc = -1; 
                }
                else
                {
                    cout << "COMMAND: " << buf << endl;
                    cli->parse_client_command(string(buf), serv);
                }
            }
        }
    }
}

/* Server commands */
void server_loop(Server *serv, char *buf, int bufSize, int *maxfds, timeval *tv)
{
    serv->readSockets = fd_set(serv->openSockets);
    memset(buf, 0, bufSize);

    int n = select(*maxfds + 1, &serv->readSockets, NULL, NULL, tv);

    if(n < 0)
    {
        perror("select failed ");
        exit(-1);
    }
    else
    {
        // Set all available sockets to listen for incoming connections
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(serv->servers[i].inUse)
            {
                FD_SET(serv->servers[i].msgSoc, &serv->openSockets);

                if(serv->servers[i].msgSoc > *maxfds)
                {
                    *maxfds = serv->servers[i].msgSoc;
                }
            }
        }

        // Check if this is a new incoming connection
        if(FD_ISSET(serv->soc, &serv->readSockets))
        {
            if(!serv->accept_incoming())
            {
                cout << "No empty server slots!" << endl;
            }            
        }

        // Listen to all set sockets
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(serv->servers[i].inUse)
            {
                FD_SET(serv->servers[i].msgSoc, &serv->openSockets);
            }

            // set new secket descriptor
            if(serv->servers[i].msgSoc > *maxfds)
            {
                *maxfds = serv->servers[i].msgSoc; 
            }
        }

        // Check if there is a message on any socket
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(serv->servers[i].inUse)
            {
                // Check if message is waiting and read
                if(FD_ISSET(serv->servers[i].msgSoc, &serv->readSockets))
                {                    
                    if(recv(serv->servers[i].msgSoc, buf, bufSize, MSG_PEEK) == 0)
                    {
                        // close connection to client
                        string error = "Closing connection";

                        read(serv->servers[i].msgSoc, buf, bufSize);
                        
                        cout << error << endl;
                        log_msgs(error);
                        
                        serv->close_connection(i);
                        continue;
                    }
                    
                    // Kill connection if message start is invalid
                    if(buf[0] != 0x01)
                    {
                        string error = "Invalid message, killing connection. ";

                        read(serv->servers[i].msgSoc, buf, bufSize);
                        
                        cout << error << endl;
                        serv->close_connection(i);
                        
                        log_msgs(error + buf);
                        break;
                    }

                    // Wait for message end
                    if(buf[strlen(buf) - 1] == 0x04)
                    {
                        read(serv->servers[i].msgSoc, buf, bufSize);

                        // Log the rec command
                        log_msgs(buf);

                        cout << "SERVER: " << buf << endl;
                        serv->parse_server_command(serv->rm_msg_tokens(string(buf)), i);
    
                    }
                }
            }
        }
    }
}


// Main program loop
int main(int argc, char* argv[])
{
    // Validate that the client has the server connection details
    if(argc < 3)
    {
        printf("Usage: ./server <\"Server port\"> <\"Client port\"><\"Host name(optional)\">\n");
        exit(-1);
    }
    int serverPort = atoi(argv[1]);
    int clientPort = atoi(argv[2]);

    struct timeval tv;                  // Struct to configure the select() waiting time
    int bufCliSize = 5002;              // Buffer size for client
    int bufServSize = 5002;             // Buffer size for servers
    Server serv(serverPort);            // Server class
    Client cli(clientPort);             // Socket to listen for a client
    char bufCli[bufCliSize];            // Buffer to read from
    char bufServ[bufServSize];          // Buffer to read from
    int maxfdsCli = 0;                  // Max filedescriptors
    int maxfdsServ = 0;                 // Max filedescriptors

    // Set wait time for messages
    tv.tv_sec = 0;
    tv.tv_usec = 500000;
    
    // Setup client listening
    // Get client socket
    cli.soc = open_soc(clientPort);

    // bind to address
    bind_socket(&cli.conn, cli.soc, cli.port);

    // listen for client
    listen_socket(&maxfdsCli, cli.soc, &cli.openSockets);

    // Setup server listening
    // Get server socket
    serv.soc = open_soc(serverPort);

    // bind to address
    bind_socket(&serv.conn, serv.soc, serv.port);

    // listen for server
    listen_socket(&maxfdsServ, serv.soc, &serv.openSockets);

    // ignore the interupt when trying to send to a server that may DC mid transmition
    signal(SIGPIPE, SIG_IGN);

    // set the group name if provided
    if(argc >= 4)
        serv.groupName = argv[3];

    while(true)
    {
        // Client logic
        client_loop(&cli, bufCli, bufCliSize, &maxfdsCli, &serv, &tv);

        // Server logic (bots)
        server_loop(&serv, bufServ, bufServSize, &maxfdsServ, &tv);

        // Get connection details for unauthenticated servers
        serv.get_conn_details();

        // global keepalive for servers
        serv.keepalive();
        usleep(5000);
    }
    
    return 0;
}
