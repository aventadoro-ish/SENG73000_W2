const WebSocket = require("ws");

const server = new WebSocket.Server({
  port: 8080
});

server.on("connection", function (socket) {
  console.log("Webpage connected");

  socket.send("Connected to the LiF WebSocket server");

  socket.on("message", function (message) {
    console.log("Received:", message.toString());
  });

  socket.on("close", function () {
    console.log("Webpage disconnected");
  });
});

console.log("WebSocket server running on port 8080");