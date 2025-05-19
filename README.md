# 🚨 Emergency Management System

Sistema distribuito per la gestione e il monitoraggio di emergenze, con backend in C multi-thread, comunicazione tramite code POSIX e frontend React/WebSocket.

---

## Indice

- [Descrizione](#descrizione)
- [Funzionalità](#funzionalità)
- [Architettura](#architettura)
- [Requisiti](#requisiti)
- [Installazione](#installazione)
- [Configurazione](#configurazione)
- [Utilizzo](#utilizzo)
- [Dettagli tecnici](#dettagli-tecnici)
- [Logging](#logging)
- [Testing](#testing)
- [Autori](#autori)

---

## Descrizione

Emergency Management System è una piattaforma per la simulazione e gestione di emergenze in tempo reale. Il backend, scritto in C, gestisce parsing di configurazioni, code di emergenze, assegnazione automatica dei soccorritori e logging avanzato. Il frontend React consente il monitoraggio live tramite WebSocket.

---

## Funzionalità

- Parsing automatico di file di configurazione (`env.conf`, `emergency_types.conf`, `rescuers.conf`)
- Gestione thread-safe di emergenze tramite coda circolare
- Scheduler per assegnazione automatica dei soccorritori alle emergenze
- Digital twin per ogni soccorritore (thread dedicato)
- Logging avanzato su file e via TCP (per dashboard)
- Dashboard React per visualizzazione in tempo reale di emergenze e soccorritori
- Client C per invio emergenze manuali o da file
- Supporto a centinaia di emergenze e soccorritori

---

## Architettura

```
+-------------------+         +-------------------+         +----------------------+
|    Client C       | <-----> |   Backend C       | <-----> |   Node.js TCP/WS     |
| (Invio emergenze) |   MQ    | (Gestione logica) |   TCP   | (Bridge WebSocket)   |
+-------------------+         +-------------------+         +----------------------+
                                                                  |
                                                                  | WebSocket
                                                                  v
                                                        +----------------------+
                                                        |   Frontend React     |
                                                        | (Dashboard live)     |
                                                        +----------------------+
```

- **Backend C**: parsing, gestione emergenze, scheduler, logging, digital twin soccorritori.
- **Client C**: invio emergenze via message queue POSIX.
- **Node.js**: bridge TCP <-> WebSocket per la dashboard.
- **Frontend React**: visualizzazione live di emergenze e soccorritori.

---

## Requisiti

- **Linux** (testato su Ubuntu/WSL)
- **GCC** (compilatore C)
- **POSIX message queue** (`-lrt` se necessario)
- **Node.js** (>= 16) e **npm** (per frontend e server bridge)
- **Web browser** moderno

---

## Installazione

### 1. Clona il repository

```sh
git clone https://github.com/DaPrato4/ProgettoLaboratorio2.git
cd progetto-lab2
```

### 2. Compila il backend C

```sh
make
```

Gli eseguibili saranno in `build/`.

### 3. Installa le dipendenze frontend

```sh
cd frontend/dashboard-react
npm install
```

### 4. Installa le dipendenze del server Node.js

```sh
cd ../node-server
npm install
```

---

## Configurazione

I file di configurazione si trovano in `conf/`:

- **env.conf**: parametri ambiente (nome coda, dimensioni griglia)
  ```
  queue=emergenze123
  height=300
  width=400
  ```
- **emergency_types.conf**: tipi di emergenza e requisiti soccorritori
  ```
  [Terremoto] [2] Pompieri:4,10;Ambulanza:3,5;Protezione Civile:5,12;
  ```
- **rescuers.conf**: tipi e quantità di soccorritori
  ```
  [Pompieri][5][20][100;200]
  ```

Modifica questi file per adattare il sistema alle tue esigenze.

---

## Utilizzo

### 1. Avvia il server Node.js (bridge TCP/WebSocket)

```sh
cd frontend/node-server
node server.js
```

### 3. Avvia la dashboard React

```sh
cd ../dashboard-react
npm run dev
```

### 2. Avvia il backend

```sh
make run
```

Visita [http://localhost:5173](http://localhost:5173) nel browser.

### 4. Invia emergenze

- **Singola emergenza**:
  ```sh
  ./build/client <nome_emergenza> <x> <y> <delay>
  ```
  Esempio:
  ```sh
  ./build/client Terremoto 120 45 60
  ```

- **Da file**:
  ```sh
  ./build/client -f <file_path>
  ```
  Esempio:
  ```sh
  ./build/client -f emergencies.txt
  ```

- **Dalla dashboard**: usa il form per creare emergenze in tempo reale.

---

## Dettagli tecnici

- **src/**: codice sorgente C (main, parser, queue, scheduler, logger, digital twin)
- **include/**: header file C
- **frontend/dashboard-react/**: codice React (dashboard)
- **frontend/node-server/**: server Node.js per bridge TCP/WebSocket
- **build/**: eseguibili compilati
- **system.log**: log di sistema

### Moduli principali C

- `main.c`: entry point, avvia logger, parsing, thread, scheduler
- `parser_*.c`: parsing file di configurazione
- `emergency_queue.c`: coda circolare thread-safe delle emergenze
- `scheduler.c`: thread che assegna soccorritori alle emergenze
- `rescuer.c`: digital twin dei soccorritori (thread)
- `logger.c`: logging su file e TCP
- `mq_receiver.c`: ricezione emergenze via message queue POSIX

---

## Logging

- Tutti gli eventi vengono loggati in `system.log` nella root del progetto.
- Se il server TCP è attivo, i log vengono inviati anche via TCP per la dashboard.
- Il logging è thread-safe e non blocca il backend.

---

## Testing

- **Manuale**: invia emergenze con il client, verifica la dashboard e i log.
- **Automatizzato**: puoi usare file di test (`emergencies.txt`) per simulare molte emergenze.
- **Debug**: controlla `system.log` e la console per messaggi di errore.

---

## Autori

- **Da Prato Gabriele**  
  [email](mailto:daprato363@gmail.com)  
  [github](https://github.com/DaPrato4)

---
