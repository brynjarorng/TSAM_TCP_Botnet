# TCP Client

A simple TCP client to go with my TCP bot code

## Installation
### Makefile
Open a linux shell and run `make`

This will create a single output file called `client`

### Command line
`g++ -Wall -std=c++11 *.cpp -o client`

Will compile the client and create an executable file called `client`


## Usage
```
./client <ip> <port>

<ip> - The ip (ipv4) address of the server
<low port> - Port open for client connection
```

## Available commands (Supported by the server)
Since this is only a dumb client that simpy sends the command to the server and waits for a response, technically this client supports any command.

### `CONNECT <IP> <PORT>`
Connect to a server with a specific ip and port

### `GETMSG,<GROUP_ID>`
Get messages from the server for a group with the id `<GROUP_ID>`.

### `SENDMSG,<FROM>,<TO>,<MESSAGE>`
Send a message to a group. `<FROM>` is the group the message is supposed to originate from, `<TO>` is the recipient and `<MESSAGE>` is the message to send.

### `LISTSERVERS`
List all servers connected to the server that the client is connected to.

### `BCMSG,<FROM>,<TO>,<MESSAGE>`
Broadcast a message to all hosts connected to the server that the client is connected to. `<FROM>` is the group the message is supposed to originate from, `<TO>` is the recipient and `<MESSAGE>` is the message to send.

### `LEAVE,<IP>,<PORT>`
Tell the server to send the leave message to a host that has the specified `<ip>` and `<port>`.

### `STATUSREQ,<TARGET_ID>`
Send a `STATUSREQ` to all connected hosts. If any host has the id specified in `<TARGET_ID>` he should send `STATUSRESP` back.
