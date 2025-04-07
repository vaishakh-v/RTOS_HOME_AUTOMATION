// web.h
const char* htmlPage = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        font-family: Arial, sans-serif;
        margin: 0;
        padding: 20px;
        background-color: #f4f4f4;
      }
      .container {
        background-color: #fff;
        padding: 20px;
        border-radius: 10px;
        box-shadow: 0 2px 4px rgba(0, 0, 0, 0.1);
        max-width: 400px;
        margin: auto;
        margin-top: 50px;
      }
      h2 {
        text-align: center;
      }
      .button-group button {
        width: 100%;
        padding: 10px;
        margin: 10px 0;
        background-color: #0984e3;
        color: white;
        border: none;
        border-radius: 5px;
        cursor: pointer;
      }
      .button-group button.active {
        background-color: #00b894;
      }
      .status {
        margin-top: 20px;
        font-size: 18px;
      }
    </style>
  </head>
  <body>
    <div class="container">
      <h2>Hi, Parikshit</h2>
      <div class="button-group">
        <button onclick="connect()">Connect</button>
        <button onclick="disconnect()">Disconnect</button>
      </div>
      <div class="status">
        <span class="status-label">Smoke sensor is <span id="smoke">--</span></span><br>
        <span class="status-label">Touch sensor is <span id="touch">--</span></span>
      </div>
    </div>
    <script>
      function connect() {
        console.log("Connect button pressed.");
      }
      function disconnect() {
        console.log("Disconnect button pressed.");
      }
    </script>
  </body>
  </html>
  )rawliteral";
  