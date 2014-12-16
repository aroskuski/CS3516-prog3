Andrew Roskuski
Adam Ansel

To build the client and server, run make in this directory.

Running the server:
./server

Notes: Uses port 5000.  All stored photos and log files go in the working directory.
Each connection to the server is considered a new client.  Client ids are assigned
sequentially starting at 1. Photo numbers for each client are assigned sequentially starting at 1.
Uses fork() to implement concurrency.

Runneing the client:
./client <serveraddress> <client id> <number of photos>

Notes: Uses select() to implement the timeout.  Timeout is 100 milliseconds.
Photo numbers are assigned sequentially starting at 1. All photos are assumed to be 
in the working directory.  The client will fail if a photo file is missing.
Logs are created in the working directory.


General Note: Frame sequence numbers are assigned independently of packet sequence numbers. 
Sequence numbers are never explictly reset.
