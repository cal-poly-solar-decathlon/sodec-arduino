# Makefile for CPE464 program 2

CC= gcc
CFLAGS= -g -Wall -Werror

# The  -lsocket -lnsl are needed for the sockets.
# The -L/usr/ucblib -lucb gives location for the Berkeley library needed for
# the bcopy, bzero, and bcmp.  The -R/usr/ucblib tells where to load
# the runtime library.

# The next line is needed on Sun boxes (so uncomment it if your on a
# sun box)
# LIBS =  -lsocket -lnsl

# For Linux/Mac boxes uncomment the next line - the socket and nsl
# libraries are already in the link path.
LIBS =

all:   tcp_server

tcp_server: tcp_server.c networks.h
	$(CC) $(CFLAGS) -o server tcp_server.c ClientTable.c $(LIBS)

clean:
	rm -rf server cclient *.o *.dSYM
