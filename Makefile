all: client server

client: client.c
	gcc -m32 -o client client.c

server: server.c
	gcc -m32 -o server server.c

clean:
	rm client server
