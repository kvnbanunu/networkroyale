build-all: server client
	echo "server and client built successfully"

server:
	gcc src/server.c src/socket.c src/args.c src/usage.c -o server

client:
	gcc src/client.c src/socket.c src/args.c src/usage.c -lncurses -o client

rs: server
	./server

rc: client
	./client

clean:
	rm -f server client
