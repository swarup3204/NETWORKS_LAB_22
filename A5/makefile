library:mysocket.o
	ar rcs libmsocket.a mysocket.o

mysocket.o:mysocket.c mysocket.h
	gcc-12 -c mysocket.c -I.
	
clean:
	rm libmsocket.a mysocket.o

