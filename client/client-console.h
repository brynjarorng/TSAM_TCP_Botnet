#ifndef FUNCTIONS_H_INCLUDED
#define FUNCTIONS_H_INCLUDED

#include <iostream>
#include <string>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <time.h>
#include <iomanip>
#include <string.h>
#include "Client.h"

// Create TCP socket
int create_socket();

// Connect to a server
void connect_to_server(int soc, Client *cli);

// Setup server conenction
void setup_serv_conn(int port, std::string serverAddr, Client *cli);

//Validate that the requested command exists
bool validate_command(std::string command);

// Print the current timestamp
void print_timestamp();


#endif