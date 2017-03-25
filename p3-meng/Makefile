all: cube

cube: cube.c
	gcc -g cube.c wizard.c -lreadline -lhistory -lncurses -lpthread -o cube

clean:
	rm -f *.o cube
