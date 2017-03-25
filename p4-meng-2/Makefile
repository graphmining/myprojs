all: mystat myls mytail mysearch

myls: myls.o
	gcc -o myls myls.c

mystat: mystat.o
	gcc -o mystat mystat.c
	
mytail: mytail.o
	gcc -o mytail mytail.c

mysearch: mysearch.o
	gcc -o mysearch mysearch.c

clean:
	rm -f *o *~
