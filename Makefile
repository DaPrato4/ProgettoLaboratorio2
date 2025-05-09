CC = gcc
CFLAGS = -Wall -Iinclude
SRC = src/main.c src/parser_emergency.c src/parser_env.c src/parser_rescuers.c
OBJ = $(SRC:.c=.o)
TARGET = eseguibile.o

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $(TARGET)

clean:
	rm -f $(TARGET) src/*.o

.PHONY: all clean