const WebSocket = require('ws');

const wss = new WebSocket.Server({ host: '0.0.0.0', port: 8080 });

// Create a set to keep track of all connected clients
const clients = new Set();

wss.on('connection', ws => {
  // Add the new connection to our set
  clients.add(ws);

  ws.on('message', message => {
    console.log('received: %s', message);
    // Broadcast the message to all clients
    clients.forEach(client => {
      if (client.readyState === WebSocket.OPEN) {
        client.send(message);
      }
    });
  });

  ws.on('close', () => {
    // Remove the connection from our set when it is closed
    clients.delete(ws);
  });
});