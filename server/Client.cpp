#include "Client.h"

#define BACKLOG 0    // Max number of waiting connections

// parse the client command and execute it
void Client::parse_client_command(std::string buf, Server *serv)
{
    std::string message;            // Var for the message response

    // Send list servers to client
    if(buf.find("LISTSERVERS") != std::string::npos)
    {
        std::string serverList = serv->create_server_list();
        write(msgSoc, serverList.c_str(), serverList.length());
        return;
    }

    // Get all messages for a specific group
    if(buf.find("GETMSG") != std::string::npos)
    {
        send_msgs_to_client(buf, serv);
        return;
    }

    // Send a message to a specific group
    if(buf.find("SENDMSG") != std::string::npos)
    {
        message = "Sending message...";
        write(msgSoc, message.c_str(), message.length());

        // Need to add the from field into the message
        auto splitBuf = split_comma(buf);   // Vector for the split buffer
        std::string newBuf;                 // New buffer for the alternate format

        // Check if the length is at least 3
        if(splitBuf.size() < 3)
            return;
        
        // add the missing value (server id) and loop through the rest of the iterator
        newBuf = "SENDMSG," + splitBuf[1] + "," + splitBuf[2] + ",";

        for(long unsigned int i = 3; i < splitBuf.size(); i++)
        {
            newBuf += "," + splitBuf[i];
        }

        serv->handle_send_msg(buf);
        return;
    }

    // Broadcast a specific msg to all connected hosts
    if(buf.find("BCMSG") != std::string::npos)
    {
        message = "Broadcasting messages";
        serv->bc_msg(buf);
        write(msgSoc, message.c_str(), message.length());
        return;
    }

    // Send the LEAVE command to a server
    if(buf.find("LEAVE") != std::string::npos)
    {
        message = "Leaving server";
        serv->send_leave(buf);
        write(msgSoc, message.c_str(), message.length());
        return;
    }

    // Connect to a specific server
    if(buf.find("CONNECT") != std::string::npos)
    {
        message = "Connecting to server";
        write(msgSoc, message.c_str(), message.length());

        std::vector<std::string> res = split_input(buf);

        // validate the length and content of the res vector
        if(res.size() == 3 && validate_ip_port(res[1], res[2]))
        {
            // returns true if connection was successfully set up
            if(serv->setup_outbound_connection(res[1], atoi(res[2].c_str())))
            {
                std::cout << "Successfully connected to " << res[1] << ":" << res[2] << std::endl;
            }
            else
            {
                std::cout << "Failed to connect to " << res[1] << ":" << res[2] << std::endl;
            }
        }
        
        return;
    }

    // STATUSREQ
    if(buf.find("STATUSREQ") != std::string::npos)
    {
        message = "Sending status request";
        write(msgSoc, message.c_str(), message.length());

        for(int i = 0; i < MAX_CLIENTS; i++)
        {
            if(serv->servers[i].inUse)
            {
                serv->send_msg(buf, i);
            }
        }
        return;
    }

    // If rec something else, send back unknown command
    if(buf.length() > 0)
        write(msgSoc, "Unknown command", std::string("unknown command").length());
}

// send the GETMSG,ID to the client
void Client::send_msgs_to_client(std::string buf, Server *serv)
{
    std::vector<std::string> splitBuf;          // Var for the split input buffer
    std::string error;                          // String used for errors
    std::string message;                        // String used for the latest message
    
    // Validate that the split buffer contains at least two items
    splitBuf = split_comma(buf);

    if(splitBuf.size() < 2)
    {
        error = "Invalid command";
        write(msgSoc, error.c_str(), error.length());
        return;
    }

    // Check if there are any messages under that id
    auto messageIt = serv->messages.find(splitBuf[1]);

    if(messageIt == serv->messages.end())
    {
        error = "No messages found";
        write(msgSoc, error.c_str(), error.length());
        return;
    }

    // else get the message
    message = messageIt->second.back();
    write(msgSoc, message.c_str(), message.length());
}
