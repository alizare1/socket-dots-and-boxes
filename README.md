# Dots and Boxes with Sockets

## Description

After running the server, it binds a TCP socket on the given port and waits for clients' connections. The server uses **select** so it can handle clients without blocking. After connecting, a client sends a number indicating how many player they want in their game (2, 3 or 4) and then waits for other players. Server keeps a list of waiting players for each room and when one of them is full, it sends a random port to the clients in that room. Upon recieving the port, players start their game by broadcasting using UDP socket on that port.

This project was done as a computer assignment for Operating Systems course at University of Tehran.

## How to run

Make the project using the makefile:

```make```

Run the server with port as argument:

```./server <port>```

Run the clients with server port as argument:

```./client <server_port>```

## How to play

Each turn the player enters three numbers to place a line: v, i ,j

* v indicates if the line is vertical or horizontal
* i indicates the row
* j indicates the column

As an example:

```0 1 2```

This means a horizontal line in second row and third column.
