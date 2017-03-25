all: memsim

memsim: memsim.o  
	gcc memsim.o -o memsim

memsim.o: memsim.c
	gcc -c memsim.c

clean:
	rm *o memsim
