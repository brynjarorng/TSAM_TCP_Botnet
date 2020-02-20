# TCP server (bot)
A simple TCP server that tries to keep a minimum of two connections in a botnet. The server can send messages, receive messages, send keepalives and check for messages waiting on other nodes as well as instructing other bots to disconnect if possible.

## Installation
### Makefile
Open a linux shell and run `make`

This will create a single output file called `tsamvgroup72`

### Command line
`g++ -Wall -std=c++11 *.cpp -o tsamvgroup72`

Will compile the server and create an executable file called `tsamvgroup72`


## Usage
```
./tsamvgroup72 <server port> <client port> (<server name>)

<server port> - Port for other servers to connect to
<client port> - Port for a client to connect to
<server name> - Optional name of the server
```

## Supported interface
Here below are the supported commands/responses and how they behave

### `LISTSERVERS,<FROM_GROUP_ID>`
Send to another server with your id to get a `SERVERS`. Used to get a list of connected servers 1-hop away.

### `SERVERS`
The response to the `LISTSERVERS` command. The first server in the list is the sender. Then come all other servers in no specific order formatted in the following way:
`SERVERS,FROM_ID,FROM_IP,FROM_PORT;SERVER1_ID,SERVER1_IP,SERVER1_PORT;...`

### `KEEPALIVE,<NO_MESSAGES>`
Send a periodic message that is used to keep track of servers that are still running correctly. This should contain the number of messages this server has for the recipient.

### `GET_MSG,<GROUP_ID>`
Get messages for a specific group from any server. Triggers a `SEND_MSG` if there are any messages on the target host.

### `SEND_MSG<FROM_ID><TO_ID><MESSAGE>`
Send a message from a specified host to a specified host. This is only sent if a server is connected with the id in `TO_ID`.

### `LEAVE,<SERVER_IP>,<PORT>`
Ask a server to disconnect from the network. Can be used to disconnect anyone from the network, not just disconnect from this server. `SERVER_IP` and `PORT` are the details of the target that should be disconnected.

### `STATUSREQ,<FROM_GROUP>`
Is used to get the total number of messages for each server stored on a target server. `FROM_GROUP` should be the group of the sender of this message.

### `STATUSRESP,<FROM>,<TO>,<(SERVER,N_MSG)>`
The response to `STATUSREQ`. `FROM` is the sender of this message, `TO` is the recipient of this message. Then there is a "tuple" containing a server id and the number of messages for that server. This tuple is only sent if there are more than 0 messages waiting.
