#ifndef SERVER_H_INCLUDED
#define SERVER_H_INCLUDED

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
#include <fstream>
#include "Client.h"
#include "Server.h"
#include <signal.h>

/* Client commands */
// Main client loop
void client_loop(Client *cli, char *buf, int bufSize, int *maxfds, Server *serv, timeval *tv);

/* Server commands */
// Main server loop
void server_loop(Server *serv, char *buf, int bufSize, int *maxfds, timeval *tv);


#endif
