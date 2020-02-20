#include "Server.h"

// Setup outbound connection
bool Server::setup_outbound_connection(std::string address, int servPort)
{
    int sockSlot = 0;           // Slot in the server list

    // Do not connect to yourself
    if(servPort == port)
    {
        return false;
    }

    // Find open server slot
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        // set sockSlot to an unused struct
        if(servers[i].inUse == false)
        {
            sockSlot = i;
            servers[i].inUse = true;
            break;
        }

        // if no open slots, return false
        if(i == MAX_CLIENTS - 1)
        {
            std::cout << "No open slots for servers!" << std::endl;
            return false;
        }
    }

    // Set connection details
    servers[sockSlot].serverAddr = address;
    servers[sockSlot].servConn.sin_family = AF_INET;
    servers[sockSlot].servConn.sin_port = htons(servPort);

    // setup socket
    setup_serv_conn(sockSlot);
    servers[sockSlot].msgSoc = create_socket();
    return connect_to_server(sockSlot);
}

// Connect to the server
// Returns true if connection was correctly set up
bool Server::connect_to_server(int sockSlot)
{
    if(connect(servers[sockSlot].msgSoc, (struct sockaddr *)&servers[sockSlot].servConn,
        sizeof(servers[sockSlot].servConn)) < 0)
    {
        // reset slot
        close_connection(sockSlot);

        return false;
    }

    numConnections++;

    return true;
}

// setup server connection
void Server::setup_serv_conn(int sockSlot)
{
    if(inet_pton(AF_INET, servers[sockSlot].serverAddr.c_str(),
        &servers[sockSlot].servConn.sin_addr) <= 0)
    {
        perror("Failed to set socket address ");
        exit(-1);
    }
}

// Create TCP socket
int Server::create_socket()
{
    int soc;            // New socket

    if((soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) <= 0)
    {
        perror("Failed to create TCP socket");
        exit(-1);
    }

    return soc;
}

// Close connection and create a new ServerInfo struct
void Server::close_connection(int sockSlot)
{
    close(servers[sockSlot].msgSoc);
    FD_CLR(servers[sockSlot].msgSoc, &openSockets);
    servers[sockSlot] = ServerInfo();
    numConnections--;
}

// Accept incoming connection if possible
// Return true on success, else false
bool Server::accept_incoming()
{
    int sockSlot = -1;          // Empty slot id

    // find empty socket
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(!servers[i].inUse)
        {
            sockSlot = i;
            servers[i].inUse = true;
            break;
        }

        // No free slots, accept, send full msg and close instantly 
        if(i == 4)
        {
            int tmpSock = accept(soc, (struct sockaddr *)&conn, &connLen);
            if(tmpSock > 0)
            {
                write(soc, fullError.c_str(), fullError.length());
                close(tmpSock);
            }
            return false;
        }
    }

    // Try to accept incoming connection
    if((servers[sockSlot].msgSoc = accept(soc, (struct sockaddr *)&conn, &connLen)) > 0)
    {
        // Get all necessary info from the requesting server, else close the connection
        //get_host_details(sockSlot);
        std::cout << "Server connected! " << std::endl;
    }
    else
    {
        std::cout << "Failed to connect! " << std::endl;
    }

    // Get the ip and port of the incoming connection
    struct sockaddr_in* pV4Addr = (struct sockaddr_in*)&conn;
    struct in_addr ipAddr = pV4Addr->sin_addr;
    in_port_t servPort = pV4Addr->sin_port;
    char address[INET_ADDRSTRLEN];
    inet_ntop( AF_INET, &ipAddr, address, INET_ADDRSTRLEN );

    std::string connDetails;
    connDetails = "IP: " + std::string(address) + ":" + std::to_string(servPort) + "\n";

    std::cout << connDetails;
    log_msgs(connDetails);

    // Add to the num servers connected
    numConnections++;

    // If incoming connection was from this server, disconnect
    if(port == servPort)
    {
        close_connection(sockSlot);
        return false;
    }

    return true;
}

// Create the server list
std::string Server::create_server_list()
{
    std::string serverList = "SERVERS,";         // string to build server list

    // Put the host first in the map
    serverList += groupName + "," + ip + "," + std::to_string(port) + ";";

    // Add all other servers to the string
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(servers[i].inUse)
        {
            serverList += servers[i].id + "," + servers[i].ip + "," + std::to_string(servers[i].port) + ";";
        }
    }

    return serverList;
}

// Send SERVERS response to LISTERVERS
void Server::send_listservers_resp(int sockSlot)
{
    std::string serverList = create_server_list();      // Server list (formatted)

    send_msg(serverList, sockSlot);
}

// Handle incoming server commands
void Server::parse_server_command(std::string buf, int sockSlot)
{
    // LISTSERVERS
    if(buf.find("LISTSERVERS") != std::string::npos)
    {
        send_listservers_resp(sockSlot);
        return;
    }

    // SERVERS response
    if(buf.find("SERVERS") != std::string::npos)
    {
        set_host_details(buf, sockSlot);
        return;
    }

    // LEAVE server with ip and port
    if(buf.find("LEAVE") != std::string::npos)
    {
        handle_leave(buf);
        return;
    }

    // GET_MSG
    if(buf.find("GET_MSG") != std::string::npos)
    {
        send_msg_command(buf, sockSlot);
        return;
    }

    // KEEPALIVE
    if(buf.find("KEEPALIVE") != std::string::npos)
    {
        // Reset the disconnect timeout
        servers[sockSlot].lastContact = time(NULL);
        return;
    }

    // SEND_MSG
    if(buf.find("SEND_MSG") != std::string::npos)
    {
        // add the msg to the msg list
        handle_send_msg(buf);
        
        return;
    }

    // STATUSREQ
    if(buf.find("STATUSREQ") != std::string::npos)
    {
        send_status_resp(buf, sockSlot);
        return;
    }

    // STATUSRESP
    if(buf.find("STATUSRESP") != std::string::npos)
    {
        handle_status_req(buf, sockSlot);
        return;
    }
}

// Send a message with error handling
void Server::send_msg(std::string buf, int sockSlot)
{
    // Add the start and stop characters
    std::string command = '\1' + buf + '\4';

    // Set sent time stamp
    servers[sockSlot].lastContact = time(NULL);

    // validate that the socket is in use
    if(servers[sockSlot].inUse)
    {
        if(write(servers[sockSlot].msgSoc, command.c_str(), command.length()) < 0)
        {
            std::cout << "Failed to send to remote host" << std::endl;
            close_connection(sockSlot);
        }
    }
}

// Set connection details for a host
void Server::set_host_details(std::string buf, int sockSlot)
{
    std::vector<std::string> result;    // Results for splitting the string

    // Parse the input and split on "," and ";"(special case)
    std::stringstream ss(buf);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if(token.find(";") != std::string::npos)
        {
            if(token.find(";") != std::string::npos)
            {
                result.push_back(token.substr(0, token.find(";")));
            }
            break;
        }

        if(token.find("SERVERS") == std::string::npos)
            result.push_back(token);
    }

    // only assign if length >= 3 and ip and port are valid
    if(result.size() >= 3 && validate_ip_port(result[1], result[2]))
    {
        servers[sockSlot].id = result[0];
        servers[sockSlot].ip = result[1];
        servers[sockSlot].port = std::stoi(result[2]);

        // if not enough clients are connected, try to add more through the connected client
        auto_connect_to_host(buf);
    }
}

// Send keepalive messages to servers at a fixed inerval
void Server::keepalive()
{
    std::unordered_map<std::string, std::vector<std::string>>::iterator it;     // Iterator for the map
    std::string message;                                                        // Var for the output message

    // send a keepalive to all servers if keepalive has timed out
    if((time(NULL) - lastKeepalive) > keepaliveTimeout)
    {
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(servers[i].inUse)
            {
                // Get the number of messages for the server
                it = messages.find(servers[i].id);
                if(it == messages.end())
                    continue;

                // reset the message var and build it again
                message = "KEEPALIVE," + it->second.size();

                send_msg(message, i);
            }
        }
    }

    // If no keepalive nor msg has been rec. disconenct
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(servers[i].inUse)
        {
            if(time(NULL) - servers[i].lastContact > inactiveTimeout)
            {
                close_connection(i);
            }
        }   
    }

    lastKeepalive = time(NULL);
}

// remove start and end tokens from buffer
std::string Server::rm_msg_tokens(std::string buf)
{
    // remove start
    if(buf[0] == '\1')
    {
        buf.substr(1);
    }

    // remove end
    if(buf[buf.length() - 1] == '\4')
    {
        buf = buf.substr(0, buf.length() - 1);
    }

    return buf;
}

// Handle the LEAVE command
void Server::handle_leave(std::string buf)
{
    std::vector<std::string> result;    // Results for splitting the string

    // do not leave if number of connections is below the minimum threshold
    if(numConnections <= MIN_CONN)
        return;

    result = split_comma(buf);

    // if results contain at least 3 commands and
    // the first is an ip address and second is the port, disconnect the host
    // if he is connected
    if(result.size() > 3 && validate_ip_port(result[1], result[2]))
    {
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(servers[i].inUse
                && servers[i].ip == result[1]
                && servers[i].port == stoi(result[2]))
            {
                close_connection(i);
                break;
            }
        }
    }
}

// Send a leave MSG to a server if it is connected
void Server::send_leave(std::string buf)
{
    std::string discIp;                 // ip of the machine that should be disconnected
    int discPort;                       // port of the connection that should be disconnected
    std::vector<std::string> result;    // Result vector for the split string

    result = split_comma(buf);

    // if ip and port are valid, send the command
    if(result.size() >= 3 && validate_ip_port(result[1], result[2]))
    {
        discIp = result[1];
        discPort = stoi(result[2]);

        // loop through all servers and try to find if it is connected
        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(servers[i].inUse && servers[i].port == discPort && servers[i].ip == discIp)
            {
                close_connection(i);
                break;
            }
        }
    }
}

// Format the messages for SEN_MSG
// SEND_MSG,<FROM_GROUP>,<TO_GROUP>,<message>
// FROM_GROUP - from the <GROUP_ID> field in GET_MSG
std::string Server::format_send_msg(std::string fromGroup, std::string toGroup, std::string msg)
{
    std::string out = "SEND_MSG,";        // Output buffer
    out += fromGroup + "," + toGroup + "," + msg;
    
    return out;
}

// Send messages to the requesting host
void Server::send_msg_command(std::string buf, int sockSlot)
{
    std::string fromGroup;              // FROM_GROUP field in message
    std::string toGroup;                // TO_GROUP field in message
    std::vector<std::string> results;   // Results from splitting the string
    std::vector<std::string> msgList;   // The list of messages for the recipient

    // Split input on comma and parse all needed info
    results = split_comma(buf);

    if(results.size() < 2)
    {
        send_msg("Invalid message", sockSlot);
    }

    fromGroup = results[1];
    toGroup = servers[sockSlot].id;

    // Check if server has messages for this specific id, else send empty list of msgs
    if(messages.find(fromGroup) == messages.end())
    {
        send_msg(format_send_msg(fromGroup, toGroup, ""), sockSlot);
        return;
    }

    msgList = messages.find(fromGroup)->second;

    // check if there are messages for this bot on the server
    // Then loop and send them
    if(msgList.size() > 0)
    {
        for(long unsigned int i = 0; i < msgList.size(); i++)
        {
            std::string formattedMsg = format_send_msg(fromGroup, toGroup, msgList[i]);
            send_msg(formattedMsg, sockSlot);
        }

        // Then clear the messages
        messages.erase(fromGroup);
    }
}

// Send get_msg
void Server::send_get_msg(std::string groupId, int sockSlot)
{
    // Check if socket is in use and send the get_msg command
    if(servers[sockSlot].inUse)
    {
        std::string msg = "GET_MSG,";

        send_msg(msg+groupId, sockSlot);
    }
}

// Send the STATUSRESP response to STATUSREQ
void Server::send_status_resp(std::string buf, int sockSlot)
{
    std::string out = "STATUSRESP,";                    // buffer to send values from
    std::unordered_map<std::string,
            std::vector<std::string>>::iterator keys;   // keys in the map of messages 

    // Add this servers ID and the to server ID
    out += groupName + "," + servers[sockSlot].id + ",";

    // loop through all items in the hash map and get the number of messages for each entry
    // do not send clients with 0 messages
    keys = messages.begin();
    while(keys != messages.end())
    {
        int numMsgs = keys->second.size();
        if(numMsgs > 0)
            out += keys->first + "," + std::to_string(numMsgs);
        keys++;
    }

    // send the msg
    send_msg(out, sockSlot);
}

// Write incoming messages into the buffer
void Server::handle_send_msg(std::string buf)
{
    std::vector<std::string> splitBuf = split_comma(buf);       // vector of the split buffer
    std::string msgAcc;                                         // Accumulator for extra splits in the msg
    std::string fromGroup;                                      // ID of the sender
    std::string toGroup;                                        // Original msg sender
    std::vector<std::string> vec;                               // Vector for the string to be inserted into the buffer

    // check if the length of the buffer matches the length of of the message in the spec
    if(splitBuf.size() < 4)
    {
        return;
    }

    fromGroup = splitBuf[1];
    toGroup = splitBuf[2];

    // add rest of msg if it has been split
    msgAcc = splitBuf[3];

    if(splitBuf.size() > 4)
    {
        for(long unsigned int i = 4; i < splitBuf.size(); i++)
        {
            msgAcc += splitBuf[i];
        }
    }

    // check if the recipient is connected and send to him, else store it locally
    // Do not send to a server with the same name, might start a message loop
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(servers[i].inUse && servers[i].id == toGroup && toGroup != fromGroup)
        {
            std::string message = "SEND_MSG," + fromGroup + "," + toGroup + "," +  msgAcc;
            send_msg(message, i);
            log_msgs(message);
            return;
        }
    }

    // Store message, need to check if there is already something with this id
    if(messages.find(toGroup) == messages.end())
    {
        // No id found
        vec.push_back(msgAcc);


        messages.insert(std::pair<std::string, std::vector<std::string>>(toGroup, vec));
    }
    else
    {
        // Id already in list
        // get the old vector and add to it before puting it back
        vec = messages.find(toGroup)->second;
        vec.push_back(msgAcc);

        messages.erase(toGroup);

        messages.insert(std::pair<std::string, std::vector<std::string>>(toGroup, vec));
    }
}

// If recv message that contains this servers id and some msgs, fetch them
void Server::handle_status_req(std::string buf, int sockSlot)
{
    auto splitBuf = split_comma(buf);       // Vector for the split buffer

    // Validate that the msg contains the minimum number of elements for this command to do anything
    // STATUSRESP,FROM,TO,<s1,qty> = 5
    if(splitBuf.size() < 5)
        return;
    
    // Iterate through the message and check if there are any messages for this server
    // Walk through it in pairs since messages come as <server,num_msgs>
    long unsigned int pos = 3;

    while(pos < splitBuf.size() - 1)
    {
        // if not this server, skip 2 forward
        if(splitBuf[pos] != groupName)
        {
            pos += 2;
            continue;
        }
        
        // Else get the waiting messages and return
        pos++;

        if(pos < splitBuf.size())
        {
            send_get_msg(groupName, sockSlot);
            return;
        }
    }
}

// Try to set server info, if it fails too many times, dc the server
void Server::get_conn_details()
{
    // Check if all sockets have the conn details set
    // Only check if there is nothing set, the socket is in use and there have
    // passed at least a few seconds
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(servers[i].id.length() == 0 && servers[i].inUse && (time(NULL) - servers[i].lastContact) > 3)
        {
            // only send msg if maximum retries has not been exceeded
            if(servers[i].contactTry < maxNumberOfConnectionTries)
            {
                send_msg("LISTSERVERS," + groupName, i);
                servers[i].contactTry++;
                servers[i].lastContact = time(NULL);
            }
            else
            {
                std::string error = "Failed to get connection details";

                std::cout << error << std::endl;
                log_msgs(error);

                close_connection(i);
            }
        }
    }
}

// Broadcast msg to all connected hosts
void Server::bc_msg(std::string buf)
{
    std::string msg;            // correctly formatted msg to broadcast
    auto splitBuf = split_comma(buf);

    // MSG must contain:
    // BCMSG, FROM, TO, MSG
    if(splitBuf.size() < 4)
    {
        return;
    }

    // format the msg
    msg = "SEND_MSG," + splitBuf[1] + "," + splitBuf[2] + "," + splitBuf[3];

    // send the msg
    for(int i = 0; i < MAX_CLIENTS; i++)
    {
        if(servers[i].inUse)
        {
            send_msg(msg, i);
        }
    }
}

// Try to automatically join another host
void Server::auto_connect_to_host(std::string buf)
{
    // Only try to connect if there is only a single host connected
    if(numConnections == 1)
    {
        // Split string on ";" and skip the first since that is the already connected host
        std::vector<std::string> result;    // Results for splitting the string

        // Parse the input and split on "," and ";"(special case)
        std::stringstream ss(buf);
        std::string token;
        while (std::getline(ss, token, ';'))
        {
            result.push_back(token);
        }

        // loop through all results except the connected server
        for(long unsigned int i = 1; i < result.size(); i++)
        {
            auto splitToken = split_comma(result[i]);

            // validate ip and port if splitToken contains 3 items (id, ip and port)
            if(splitToken.size() == 3 && validate_ip_port(splitToken[1], splitToken[2]))
            {
                if (numConnections > 1)
                    return;
                setup_outbound_connection(splitToken[1], stoi(splitToken[2]));
            }
        }
    }
}
