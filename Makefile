# Compilatore da utilizzare
CC = gcc

# Opzioni di compilazione: -Wall abilita tutti i warning, -Iinclude aggiunge la directory 'include' per i file header
CFLAGS = -Wall -Iinclude

# File sorgenti per il programma principale
SRC_MAIN = src/main.c src/parser_emergency.c src/parser_env.c src/parser_rescuers.c src/emergency_queue.c src/mq_receiver.c src/rescuer.c src/scheduler.c src/logger.c src/emergency_status.c

# File sorgenti per il client
SRC_CLIENT = src/client.c src/parser_env.c src/logger.c

# Percorso dell'eseguibile principale
MAIN = build/eseguibile

# Percorso dell'eseguibile client
CLIENT = build/client

# Target di default: compila sia il programma principale che il client
all: $(MAIN) $(CLIENT)

# Regola per compilare il programma principale
$(MAIN): $(SRC_MAIN) | build
	$(CC) $(CFLAGS) $(SRC_MAIN) -o $(MAIN)

# Regola per compilare il client
$(CLIENT): $(SRC_CLIENT) | build
	$(CC) $(CFLAGS) $(SRC_CLIENT) -o $(CLIENT)

# Regola per creare la directory di build se non esiste
build:
	mkdir -p build

# Regola per pulire i file compilati
clean:
	rm -f build/*

# Regola per eseguire il programma principale
run: $(MAIN)
	./$(MAIN)

# Target che non corrispondono a file
.PHONY: all clean run build