const net = require('net');                 // Modulo TCP nativo Node.js
const WebSocket = require('ws');            // Libreria WebSocket per Node.js
const { spawn } = require('child_process'); // Modulo per eseguire processi figli

// Crea un server WebSocket sulla porta 8080 per React
const wss = new WebSocket.Server({ host: '0.0.0.0', port: 8080 });

wss.on('connection', ws => {                // Quando un client React si connette
  console.log('React client connected');

  ws.on('close', () => {                    // Quando React si disconnette
    console.log('React client disconnected');
  });

  ws.on('message', msg => {                 // Quando riceve un messaggio da React
    // msg è il JSON ricevuto da React, { name, x, y, delay }
    const data = JSON.parse(msg);
    if (data.event === 'EMERGENCY_CREATE') {
      // Lancia il client C con i parametri
      const path = require('path');
      const clientPath = path.join(__dirname, '..', '..', 'build', 'client');
      const args = [String(data.name), String(data.x), String(data.y), String(data.delay)];
      console.log("Eseguibile C path:", clientPath, "Args:", args);

      const child = spawn(clientPath, args, {
        cwd: path.join(__dirname, '..', '..'), // <-- imposta la working directory sulla root del progetto!
      });

      child.stdout.on('data', d => console.log(`client: ${d}`));
      child.stderr.on('data', d => console.error(`client error: ${d}`));
      child.on('close', (code, signal) => console.log(`client exited with code ${code}, signal ${signal}`));
      child.on('error', (err) => {
        console.error("Errore nel lancio del client:", err);
      });
    }
  });
});

// Crea un server TCP sulla porta 9000 per ricevere dati dall' app C
const tcpServer = net.createServer(socket => {
  console.log('C client connected');

  // Quando riceve dati dal client C
  socket.on('data', data => {
    const messages = data.toString().split("\n").filter(m => m.trim() !== "");

    messages.forEach(msg => {
      console.log('📥 Received from C:', msg);
      wss.clients.forEach(client => {
        if (client.readyState === WebSocket.OPEN) {
          client.send(msg); // Invia un JSON alla volta
          console.log("📤 Sent to React client:", msg);
        }
      });
    });
  });

  socket.on('close', () => {                   // Quando client C si disconnette
    console.log('C client disconnected');
  });
});

// Fa partire il server TCP sulla porta 9000
tcpServer.listen(9000, () => {
  console.log('TCP server listening on port 9000');
});