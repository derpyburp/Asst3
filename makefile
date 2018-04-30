CFLAGS = -g -Wall -Werror

all: libnetfiles.o
	gcc $(CFLAGS) -o  

libnetfiles.o: libnetfiles.c
	gcc $(CFLAGS) -c -o libnetfiles.o libnetfiles.c
clean:
	rm libnetfiles.o  
