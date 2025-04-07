#ifndef WEB_INTERFACE_H
#define WEB_INTERFACE_H

const char MAIN_page[] PROGMEM = R"rawliteral(
  <!DOCTYPE html>
  <html>
  <head>
    <title>ESP32 Smart Room</title>
    <meta name="viewport" content="width=device-width, initial-scale=1" />
    <style>
      body { font-family: Arial; background: #f0f0f0; padding: 20px; }
      h1 { color: #333; }
      .status { padding: 10px; background: white; margin: 10px 0; border-radius: 5px; }
      button { padding: 10px 20px; font-size: 16px; border: none; background: #2196F3; color: white; border-radius: 5px; cursor: pointer; }
      button:hover { background: #0b7dda; }
    </style>
  </head>
  <body>
    <h1>ESP32 Smart Room Dashboard</h1>
    <div class="status" id="roomStatus">Room Status: --</div>
    <div class="status" id="personStatus">Person Inside: --</div>
    <div class="status" id="monitorStatus">Light Monitoring: --</div>
    <div class="status" id="intrusionStatus">Intrusion Alert: --</div>
    <div class="status" id="unauthStatus">Unauthorized Access: --</div>
    <button onclick="toggleLight()">Toggle Light Monitoring</button>
  
    <script>
      function fetchStatus() {
        fetch("/status")
          .then(response => response.json())
          .then(data => {
            document.getElementById("roomStatus").innerText = "Room Mode: " + data.roomMode;
            document.getElementById("personStatus").innerText = "Person Inside: " + data.personInside;
            document.getElementById("monitorStatus").innerText = "Light Monitoring: " + data.monitorLight;
            document.getElementById("intrusionStatus").innerText = "Intrusion Alert: " + data.intrusionAlert;
            document.getElementById("unauthStatus").innerText = "Unauthorized Access: " + data.unauthAccess;
          });
      }
  
      function toggleLight() {
        fetch("/toggle-light", { method: "POST" })
          .then(() => fetchStatus());
      }
  
      setInterval(fetchStatus, 2000);
      window.onload = fetchStatus;
    </script>
  </body>
  </html>
  )rawliteral";
  

#endif
