#include "functions.h"

// Bind client listening socket
void bind_socket(sockaddr_in *conn, int soc, int port)
{
    // Set connection typpe and bind to address
    conn->sin_family = AF_INET;
    conn->sin_addr.s_addr = INADDR_ANY;
    conn->sin_port = htons(port);

    if(bind(soc, (struct sockaddr *)conn, sizeof(*conn)) < 0)
    {
        perror("Failed to bind to address");
        exit(-1);
    }
}

// Listen fot the client
void listen_socket(int *maxfds, int soc, fd_set *openSockets)
{
     if(listen(soc, BACKLOG) < 0)
    {
        std::cout << "Failed to listen on port " << std::endl;
        exit(0);
    }
    else
    {
        // Set FD set
        FD_SET(soc, openSockets);
        *maxfds = soc;
    }
}

// Open a socket for a client to connect to in order to control the server
int open_soc(int port)
{
    int set = 1;                 // Used to set sock opt
    int soc;                     // socket for connection

    // open socket
    if((soc = socket(AF_INET, SOCK_STREAM , IPPROTO_TCP)) < 0)
    {
        perror("Failed to open socket ");
        exit(-1);
    }

    // Reuse socket quickly after disconnect
    if(setsockopt(soc, SOL_SOCKET, SO_REUSEADDR, &set, sizeof(set)) < 0)
    {
        perror("Failed to set SO_REUSEADDR ");
        exit(-1);
    }

    return soc;
}

// Special splitter for splitting on whitespaces
std::vector<std::string> split_input(std::string buf)
{
    std::vector<std::string> result;  // Vector for all arts of the string

    // Use stringstream to split a string on whitespaces
    std::istringstream iss(buf);
    for(std::string s; iss >> s; )
    {
        result.push_back(s);
    }
    
    return result;
}

// Get the group id from a client command
std::string get_group_id(std::string buf)
{
    std::vector<std::string> splitString = split_input(buf);
    // Check if the reult vector contains more than 1 item
    if(splitString.size() <= 1)
    {
        return std::string("Invalid command!");
    }
    
    return splitString[1];
}

// Get current timestamp
std::string get_timestamp()
{
    std::stringstream ss;                           // Output buffer
    time_t currTime = time(NULL);                   // Current time var
    struct tm *localTime = localtime(&currTime);    // ISO time

    // Write into the output buffer with a fixed witdth of 2
    ss << std::setfill('0') << std::setw(2) << localTime->tm_mday << "-";
    ss << std::setfill('0') << std::setw(2) << localTime->tm_hour << ":";
    ss << std::setfill('0') << std::setw(2) << localTime->tm_min << ":";
    ss << std::setfill('0') << std::setw(2) << localTime->tm_sec;

    return ss.str();
}

// Validate ip and port are valid
bool validate_ip_port(std::string ip, std::string port)
{
    int portNum;                // Variable for the port as an int
    int minPort = 0;            // The lowest available port
    int maxPort = 65535;        // The highest available port
    struct sockaddr_in sa;      // Return ip address value

    // Port could contain ";", remove that
    if(port.find(";") != std::string::npos)
        port = port.substr(0, port.length() - 1);

    // validate ip
    if(inet_pton(AF_INET, ip.c_str(), &sa) < 1)
        return false;

    // validate that the port is a number
    for (std::size_t i = 0; i < port.length(); i++)
    {
        if(!isdigit(port[i]))
            return false;
    }

    // Check that the port is within the allowed port range
    portNum = stoi(port);

    if(portNum < minPort || portNum > maxPort)
        return false;

    return true;
}

// Split a string on: ,
std::vector<std::string> split_comma(std::string buf)
{
    std::vector<std::string> result;    // vector for all the results
    std::stringstream ss(buf);          // String stream to split
    std::string token;                  // Single token

    while (std::getline(ss, token, ',')) {
        result.push_back(token);
    }

    return result;
}

// Log messages to file
void log_msgs(std::string buf)
{
    // Log command to file
    std::string timeStamp = get_timestamp();

    std::ofstream log;
    log.open (LOGFILE, std::ios_base::app);
    log << timeStamp.c_str() << " - " << buf << std::endl;
    log.close();
}
