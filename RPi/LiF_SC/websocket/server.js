
// load .env that contains a "config" file of sorts
require("dotenv").config();
// websocket and sql libraries
const WebSocket = require("ws");
const mysql = require("mysql2/promise");

// create websocket server on port 8080
const server = new WebSocket.Server({
  port: Number(process.env.WS_PORT) || 8080 // websocket server port
});

// send a JS object to every connected webpage
function broadcast (messageObject) {
  // websockets cannot directly send a JS object so convert to string
  // use stringify
  const messageText = JSON.stringify(messageObject);

  // counts how many webpagesz receive the message (mostly terminal output use-case)
  let connectedClients = 0;

  // server.clients contains every browser currently connected to the websocket
  // loop through them
  server.clients.forEach(function (client) {
    // only send if the websocket connection is open
    if (client.readyState == WebSocket.OPEN) {
      client.send(messageText);
      connectedClients++;
    }
  });

  return connectedClients;
}

// when a webpage connects, run this event
server.on("connection", function(socket) {
  console.log("webpage connected");

  // send a test message back to the newly connected webpage
  socket.send(JSON.stringify({
    type: "connection",
    message: "connected to the LiF websocket server"
  }));

  // print any message received from the webpage
  socket.on("message", async function (message) {
    let receivedMessage;
    
    try {
      receivedMessage = JSON.parse(message.toString());

    } catch (error) {
      console.error("invalid webpage message");
      return;
    }

    // ignore messages that are not a door-change
    if(receivedMessage.type !== "door_state_changed"){
      return;
    }

    try {
      const query = `
        SELECT doors_open
        FROM elevator_state
        WHERE state_id = 1
        LIMIT 1
      `;

      // read confirmed door state from MariaDB
      const [rows] = await database.execute(query);

      // if query found nothing new, end the check
      if(rows.length === 0) {
        console.warn("elevator_state row #1 does not exist");
        return;
      }

      const doorsOpen = Number(rows[0].doors_open) === 1;

      // send confirmed state to every webpage
      const recipients = broadcast({
        type: "elevator_state",
        doors_open: doorsOpen
      });

      console.log("Broadcast confirmed door state to " + recipients + " webage(s)");

    } catch (error) {
        console.error("Door-state check failed: ", error.message);
    }
  });

  // print web page disconnected
  socket.on("close", function() {
    console.log("webpage disconnected");
  });
});

// debug/status print:
console.log("websocket server running on  port 8080");

// variable to hold MariaDB connection
let database;
// remember latest CAN entry
let lastLoggedID = 0;


// this function connects Node to MariaDB
async function connectToDB () {
  try {
    // wait for mariaDB to accept the connection using the variables within the .env file
    database = await mysql.createConnection({
      host: process.env.DB_HOST,                    /// 127.0.0.1
      port: Number(process.env.DB_PORT) || 3306,    // DB runs on port 3306
      user: process.env.DB_USER,                    // LiF_Admin
      password: process.env.DB_PASSWORD || "",      // password
      database: process.env.DB_NAME                 // lif_elevator
    });

    console.log("Connected to Lif_Elevator database");

    // read only the latest CAN entry from DB
        // Read only the newest CAN-log entry.
    const query = `
      SELECT
        log_id,
        can_id,
        direction,
        raw_byte,
        source_controller,
        logged_at
      FROM can_message_log
      ORDER BY log_id DESC
      LIMIT 1
    `;

    // execute the SQL query and wait for MariaDB to return the result
    // mysql2 returns [rows, extraInfo] so [rows] keeps only the row pulled from DB
    const [rows] =  await database.execute(query);

    if (rows.length === 0) {
      console.log("CAN message log is emppty");
      lastLoggedID = 0;

    } else {
      console.log("Newest CAN entry:");
      console.table(rows);

      // ignore existing history and begin after the current newest row?
      lastLoggedID = Number(rows[0].log_id);
    }

    console.log(`watching for entries after log#${lastLoggedID}`);

    // check every 500 ms
    setInterval(checkNewCAN, 500);

  } catch (error) {
      // if the login or query fails, print the caught error:
      console.error("Database connection error: ", error.message);
  }
} 

async function checkNewCAN () {
  try {
    const query = `
      SELECT
        log_id,
        can_id,
        direction,
        raw_byte,
        source_controller,
        logged_at
      FROM can_message_log
      WHERE log_id > ?
      ORDER BY log_id ASC
    `;

    // replace the ? placeholder with lastLoggedID
    const [rows] = await database.execute(query, [lastLoggedID]);

    // if query found nothing new, end the check
    if(rows.length === 0) {
      return;
    }

    // print the results
    console.log(`${rows.length} new can entries detected:`);
    console.table(rows);

    // broadcast each newly detected row separately
    for (const row of rows) {
      // create a structured message using relevant CAN data
      const CANMessage = {
        type: "can_message",
        log_id: Number(row.log_id),
        can_id: row.can_id,
        direction: row.direction,
        raw_byte: Number(row.raw_byte),
        source_controller: row.source_controller,
        logged_at: row.logged_at
      };

      // send the can message to every connected webpage
      const recipients = broadcast(CANMessage);

      console.log(`broadcast log #${row.log_id} to ${recipients} webpage(s)`);
    }

    // sort rows from oldest to newest
    const newestRow = rows[rows.length - 1];
    lastLoggedID = Number(newestRow.log_id);

    // print the latest entry
    console.log(`next check will begin after log #${lastLoggedID}`);

  } catch (error) {
      console.error("CAN-log failed: ", error.message);
  }
}



connectToDB();