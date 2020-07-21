.SILENT:
all: build_c build_o clean

build_o: 
	gcc -o client client.o -l pthread 
	gcc -g -o server server.o users.o channels.o -l pthread 

build_c: 
	gcc -c src/client.c
	gcc -c src/users.c -Ihdr/
	gcc -c src/channels.c -Ihdr/
	gcc -c src/server.c -Ihdr/

build_g:
	gcc -g src/client.c -l pthread 
	gcc -g src/server.c src/channels.c src/users.c -Ihdr/ -l pthread 

clean:
	rm *.o -f

run_client:
	./client $(name)

run_server:
	./server