#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include <string.h>
#include <netinet/tcp.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <unistd.h>
#include <iostream>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <iomanip>
#include <sstream>
#include <arpa/inet.h>
#include <fstream>

class Client;
#define LOGFILE "logfile.log"   // Name of the logfile

#define BACKLOG 0    // Max number of waiting connections

// Bind client listening socket
void bind_socket(sockaddr_in *conn, int soc, int port);

// Listen fot the client
void listen_socket(int *maxfds, int soc, fd_set *openSockets);

// Open a socket for a client to connect to in order to control the server
int open_soc(int port);

// Split input buffer on spaces and return a vector containing all words
std::vector<std::string> split_input(std::string buf);

// Get the group id from a client command
std::string get_group_id(std::string buf);

// Get current timestamp
std::string get_timestamp();

// Validate the ip address and port
// Return true if correct, else false
bool validate_ip_port(std::string ip, std::string port);

// Split a string on a comma: ","
std::vector<std::string> split_comma(std::string buf);

// Log input to a file
void log_msgs(std::string buf);

#endif
