#include "client-console.h"

using namespace std;

// Create TCP socket
int create_socket()
{
    int soc;

    if((soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)
    {
        perror("Failed to create TCP socket");
        exit(-1);
    }

    return soc;
}

// Connect to the server
void connect_to_server(int soc, Client *cli)
{
    if(connect(soc, (struct sockaddr *)&cli->servConn, sizeof(cli->servConn)) < 0)
    {
        perror("Failed to connect to the server ");
        exit(-1);
    }
}

// setup server connection
void setup_serv_conn(int port, string serverAddr, Client *cli)
{
    cli->servConn.sin_family = AF_INET;
    cli->servConn.sin_port = htons(port);

    if(inet_pton(AF_INET, serverAddr.c_str(), &cli->servConn.sin_addr) <= 0)
    {
        perror("Failed to set socket address ");
        exit(-1);
    }
}

// Print the timestamp
void print_timestamp()
{
    time_t currTime = time(NULL);
    struct tm *localTime = localtime(&currTime);
    string day;
    string hour;
    string minute;
    string second;

    cout << setfill('0') << setw(2) << localTime->tm_mday << "-";
    cout << setfill('0') << setw(2) << localTime->tm_hour << ":";
    cout << setfill('0') << setw(2) << localTime->tm_min << ":";
    cout << setfill('0') << setw(2) << localTime->tm_sec;
}

int main(int argc, char *argv[])
{
    struct timeval tv;              // Struct to configure the select() waiting time
    Client cli;                     // Client connection info
    string input;                   // User command input
    string timestamp;               // The timestamp
    fd_set openSockets;             // FD_set of open sockets
    char buf[5002];                 // Buffer to recv messages


    // Validate that the client has the server connection details
    if(argc != 3)
    {
        printf("Usage: ./client <\"ip\"> <\"port\">\n");
        exit(-1);
    }

    // Get connection details
    cli.serverAddr = argv[1];
    cli.port = atoi(argv[2]);

    // setup server connection
    setup_serv_conn(cli.port, cli.serverAddr, &cli);

    // create the socket
    cli.soc = create_socket();

    // connect to server
    connect_to_server(cli.soc, &cli);

    // Set wait time for messages
    tv.tv_sec = 2;
    tv.tv_usec = 0;

    // Watch sockets
    FD_SET(cli.soc, &openSockets);

    while(true)
    {
        fd_set readSockets = fd_set(openSockets);   // Changeable list of sockets
        memset(buf, 0, sizeof(buf));

        // Read input w. format
        cout << ">";
        getline(cin, input);

        // If q, then halt
        if(input == "q")
        {
            break;
        }

        // Else send command to server
        print_timestamp();
        cout << ">" << input << endl;
        write(cli.soc, input.c_str(), input.length());

        // Wait for message to be recvd
        int n = select(cli.soc + 1, &readSockets, NULL, NULL, &tv);
        if(n < 0)
        {
            perror("select failed ");
            exit(-1);
        }
        else 
        {
            if(FD_ISSET(cli.soc, &readSockets))
            {
                // Read message
                if(read(cli.soc, buf, sizeof(buf)) == 0)
                {
                    cout << "Server disconnected" << endl;
                    break;
                }
                else
                {
                    print_timestamp();
                    cout << " - " << buf << endl;
                }
            }
        }

    }

    close(cli.soc);
    return 0;
}