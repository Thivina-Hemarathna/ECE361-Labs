MAKE = server client

all: ${MAKE}
server: server.c
    gcc server.c -o server
deliver: client.c
    gcc client.c -o client
clean:
    rm -f ${MAKE}
