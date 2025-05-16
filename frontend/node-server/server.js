const net = require('net');                 // Modulo TCP nativo Node.js
const WebSocket = require('ws');            // Libreria WebSocket per Node.js

// Crea un server WebSocket sulla porta 8080 per React
const wss = new WebSocket.Server({ port: 8080 });

wss.on('connection', ws => {                // Quando un client React si connette
  console.log('React client connected');

  ws.on('close', () => {                    // Quando React si disconnette
    console.log('React client disconnected');
  });
});

// Crea un server TCP sulla porta 9000 per ricevere dati dalla tua app C
const tcpServer = net.createServer(socket => {
  console.log('C client connected');

  // Quando riceve dati dal client C
  socket.on('data', data => {
    console.log('Received from C:', data.toString());

    // Inoltra i dati a tutti i client React connessi
    wss.clients.forEach(client => {
      if (client.readyState === WebSocket.OPEN) {   // Solo client aperti
        client.send(data.toString());                // Manda messaggio WebSocket
      }
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