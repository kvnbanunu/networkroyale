build-all: server client
	echo "server and client built successfully"

server:
	gcc src/server.c src/setup.c src/game.c -o debug/server

client:
	gcc src/main.c src/setup.c src/args.c src/game.c src/render.c -lncurses -lSDL2 -o debug/client

rs: server
	./server

rc: client
	./client

clean:
	rm -f debug/server debug/client
