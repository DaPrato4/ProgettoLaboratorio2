CC = gcc
CFLAGS = -Wall -Iinclude

SRC_MAIN = src/main.c src/parser_emergency.c src/parser_env.c src/parser_rescuers.c src/emergency_queue.c src/mq_receiver.c src/rescuer.c src/scheduler.c
SRC_CLIENT = src/client.c src/parser_env.c

OBJ_MAIN = $(SRC_MAIN:.c=.o)
OBJ_CLIENT = $(SRC_CLIENT:.c=.o)

MAIN = eseguibile.o
CLIENT = client.o

all: $(MAIN) $(CLIENT)

$(MAIN): $(SRC_MAIN)
	$(CC) $(CFLAGS) $(SRC_MAIN) -o $(MAIN)

$(CLIENT): $(SRC_CLIENT)
	$(CC) $(CFLAGS) $(SRC_CLIENT) -o $(CLIENT)

clean:
	rm -f $(MAIN)  src/*.o

.PHONY: all clean