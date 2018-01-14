CFLAGS=-ggdb -Wall -Werror -std=c99 -I.

all: multisock
multisock: multisock.o test_multisock.o
clean:
	rm multisock *.o
