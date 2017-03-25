all: block prefetch cache inode timer

.PHONY: block prefetch cache inode clean

block: block.c
	gcc -o block block.c

prefetch: prefetch.c
	gcc -o prefetch prefetch.c

cache: cache.c
	gcc -o cache cache.c

inode: inode.c
	gcc -o inode inode.c

timer: timer.c
	gcc -o timer timer.c

clean: 
	rm -f *.o block prefetch cache inode timer
