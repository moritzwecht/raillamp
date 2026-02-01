#include "mywebserver.h"
#include "leds.h"
#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>

WebServer server(80);

// Status Variablen
String currentStatus = "Bereit";
String currentError = "";

// Extern aus leds.cpp
extern int currentBrightness;
extern int MAX_BRIGHTNESS;
extern CRGB targetColor;

// Extern aus main.cpp
extern unsigned long TIMEOUT;
void setTimeoutValue(unsigned long val);

void updateStatus(const String& status) {
  currentStatus = status;
}

void updateError(const String& error) {
  currentError = error;
}

// HTML + CSS + JS als String
String getHTML() {
  String html = R"rawliteral(
    <!DOCTYPE html>
    <html lang="de">
    <head>
      <meta charset="UTF-8">
      <meta name="viewport" content="width=device-width, initial-scale=1.0">
      <title>Nachtlicht</title>
      <style>
        * {
          margin: 0;
          padding: 0;
          box-sizing: border-box;
        }

        body {
          font-family: 'Segoe UI', sans-serif;
          background: #1a1a2e;
          color: #eee;
          min-height: 100vh;
          display: flex;
          justify-content: center;
          align-items: flex-start;
          padding: 30px 15px;
        }

        .container {
          width: 100%;
          max-width: 400px;
          display: flex;
          flex-direction: column;
          gap: 20px;
        }

        h1 {
          text-align: center;
          font-size: 1.8em;
          color: #ffaa55;
          text-shadow: 0 0 10px rgba(255, 170, 85, 0.3);
        }

        .card {
          background: #16213e;
          border-radius: 16px;
          padding: 20px;
          box-shadow: 0 4px 15px rgba(0,0,0,0.3);
        }

        .card h2 {
          font-size: 0.85em;
          text-transform: uppercase;
          letter-spacing: 1px;
          color: #888;
          margin-bottom: 12px;
        }

        /* Lampen-Status */
        .lamp-status {
          display: flex;
          align-items: center;
          justify-content: center;
          gap: 15px;
        }

        .lamp-icon {
          font-size: 3em;
          transition: text-shadow 0.5s ease;
        }

        .lamp-icon.on {
          text-shadow: 0 0 20px #ffaa55, 0 0 40px #ff8833;
        }

        .status-text {
          font-size: 1.2em;
          font-weight: bold;
        }

        .status-text.on { color: #ffaa55; }
        .status-text.off { color: #666; }

        /* Status & Fehler */
        .status-msg {
          font-size: 0.95em;
          color: #aaa;
          padding: 8px 12px;
          background: #0f3460;
          border-radius: 8px;
        }

        .error-msg {
          font-size: 0.95em;
          color: #ff5555;
          padding: 8px 12px;
          background: #2a1a1a;
          border-radius: 8px;
          display: none;
        }

        .error-msg.visible { display: block; }

        /* Parameter */
        .param-row {
          display: flex;
          justify-content: space-between;
          align-items: center;
          padding: 10px 0;
          border-bottom: 1px solid #0f3460;
        }

        .param-row:last-child { border-bottom: none; }

        .param-label { color: #888; }
        .param-value { font-weight: bold; color: #ffaa55; }

        /* Slider */
        .slider-row {
          padding: 12px 0;
        }

        .slider-row label {
          display: flex;
          justify-content: space-between;
          color: #888;
          margin-bottom: 8px;
          font-size: 0.9em;
        }

        input[type="range"] {
          width: 100%;
          -webkit-appearance: none;
          height: 6px;
          border-radius: 3px;
          background: #0f3460;
          outline: none;
        }

        input[type="range"]::-webkit-slider-thumb {
          -webkit-appearance: none;
          width: 20px;
          height: 20px;
          border-radius: 50%;
          background: #ffaa55;
          cursor: pointer;
          box-shadow: 0 0 8px rgba(255, 170, 85, 0.5);
        }

        /* Farb-Vorwahl */
        .color-options {
          display: flex;
          gap: 10px;
          flex-wrap: wrap;
        }

        .color-btn {
          width: 40px;
          height: 40px;
          border-radius: 50%;
          border: 2px solid transparent;
          cursor: pointer;
          transition: border-color 0.3s, transform 0.2s;
        }

        .color-btn:hover { transform: scale(1.1); }
        .color-btn.active { border-color: #fff; }

        /* Auto-Refresh */
        .refresh-info {
          text-align: center;
          font-size: 0.7em;
          color: #555;
          margin-top: 5px;
        }
      </style>
    </head>
    <body>
      <div class="container">
        <h1>🌙 Nachtlicht</h1>

        <!-- Lampen Status -->
        <div class="card">
          <h2>Status</h2>
          <div class="lamp-status">
            <div class="lamp-icon" id="lampIcon">💡</div>
            <div class="status-text off" id="statusText">AUS</div>
          </div>
        </div>

        <!-- Statusmeldung -->
        <div class="card">
          <h2>Meldung</h2>
          <div class="status-msg" id="statusMsg">Bereit</div>
          <div class="error-msg" id="errorMsg"></div>
        </div>

        <!-- Parameter -->
        <div class="card">
          <h2>Parameter</h2>
          <div class="param-row">
            <span class="param-label">Helligkeit</span>
            <span class="param-value" id="brightnessValue">0%</span>
          </div>
          <div class="param-row">
            <span class="param-label">Max. Helligkeit</span>
            <span class="param-value" id="maxBrightnessValue">30</span>
          </div>
          <div class="param-row">
            <span class="param-label">Farbe (RGB)</span>
            <span class="param-value" id="colorValue">255, 140, 60</span>
          </div>
          <div class="param-row">
            <span class="param-label">Timeout</span>
            <span class="param-value" id="timeoutValue">10s</span>
          </div>
          <div class="param-row">
            <span class="param-label">PIR Sensor 1</span>
            <span class="param-value" id="pir1Value">—</span>
          </div>
          <div class="param-row">
            <span class="param-label">PIR Sensor 2</span>
            <span class="param-value" id="pir2Value">—</span>
          </div>
        </div>

        <!-- Steuerung -->
        <div class="card">
          <h2>Steuerung</h2>

          <div class="slider-row">
            <label>
              <span>Max. Helligkeit</span>
              <span id="brightnessSliderVal">30</span>
            </label>
            <input type="range" id="brightnessSlider" min="10" max="255" value="30">
          </div>

          <div class="slider-row">
            <label>
              <span>Timeout (Sekunden)</span>
              <span id="timeoutSliderVal">10</span>
            </label>
            <input type="range" id="timeoutSlider" min="3" max="60" value="10">
          </div>

          <div class="slider-row">
            <label><span>Farbe</span></label>
            <div class="color-options">
              <div class="color-btn active" style="background:#ff8c3c" 
                   onclick="setColor(255,140,60, this)" title="Warm"></div>
              <div class="color-btn" style="background:#ffcc66" 
                   onclick="setColor(255,200,100, this)" title="Gelb-Warm"></div>
              <div class="color-btn" style="background:#ffffff" 
                   onclick="setColor(255,255,255, this)" title="Weiß"></div>
              <div class="color-btn" style="background:#ff6644" 
                   onclick="setColor(255,100,60, this)" title="Orange"></div>
              <div class="color-btn" style="background:#aa66ff" 
                   onclick="setColor(170,100,255, this)" title="Lila"></div>
              <div class="color-btn" style="background:#44aaff" 
                   onclick="setColor(70,170,255, this)" title="Blau"></div>
            </div>
          </div>
        </div>

        <div class="refresh-info">Auto-Refresh alle 2 Sekunden</div>
      </div>

      <script>
        // Auto-Refresh Status
        setInterval(fetchStatus, 2000);
        fetchStatus();

        async function fetchStatus() {
          try {
            const res = await fetch('/status');
            const data = await res.json();
            updateUI(data);
          } catch (e) {
            showError("Verbindung verloren!");
          }
        }

        function updateUI(data) {
          // Lampen Status
          const icon = document.getElementById('lampIcon');
          const text = document.getElementById('statusText');
          if (data.lightsOn) {
            icon.className = 'lamp-icon on';
            text.className = 'status-text on';
            text.textContent = 'AN';
          } else {
            icon.className = 'lamp-icon';
            text.className = 'status-text off';
            text.textContent = 'AUS';
          }

          // Parameter
          document.getElementById('brightnessValue').textContent = 
            Math.round((data.brightness / 255) * 100) + '%';
          document.getElementById('maxBrightnessValue').textContent = 
            data.maxBrightness;
          document.getElementById('colorValue').textContent = 
            data.r + ', ' + data.g + ', ' + data.b;
          document.getElementById('timeoutValue').textContent = 
            data.timeout + 's';
          document.getElementById('pir1Value').textContent = 
            data.pir1 ? '✓ Aktiv' : '— Ruhig';
          document.getElementById('pir1Value').style.color = 
            data.pir1 ? '#55ff55' : '#888';
          document.getElementById('pir2Value').textContent = 
            data.pir2 ? '✓ Aktiv' : '— Ruhig';
          document.getElementById('pir2Value').style.color = 
            data.pir2 ? '#55ff55' : '#888';

          // Status Meldung
          document.getElementById('statusMsg').textContent = data.status;
          showError(data.error);

          // Slider synchronisieren (nur wenn nicht gerade bedient)
          if (document.activeElement.id !== 'brightnessSlider') {
            document.getElementById('brightnessSlider').value = data.maxBrightness;
            document.getElementById('brightnessSliderVal').textContent = data.maxBrightness;
          }
          if (document.activeElement.id !== 'timeoutSlider') {
            document.getElementById('timeoutSlider').value = data.timeout;
            document.getElementById('timeoutSliderVal').textContent = data.timeout;
          }
        }

        function showError(msg) {
          const el = document.getElementById('errorMsg');
          if (msg && msg !== '') {
            el.textContent = '⚠️ ' + msg;
            el.className = 'error-msg visible';
          } else {
            el.className = 'error-msg';
          }
        }

        // Helligkeit Slider
        document.getElementById('brightnessSlider').addEventListener('input', function() {
          document.getElementById('brightnessSliderVal').textContent = this.value;
          fetch('/set/brightness/' + this.value);
        });

        // Timeout Slider
        document.getElementById('timeoutSlider').addEventListener('input', function() {
          document.getElementById('timeoutSliderVal').textContent = this.value;
          fetch('/set/timeout/' + this.value);
        });

        // Farbe setzen
        function setColor(r, g, b, btn) {
          document.querySelectorAll('.color-btn').forEach(b => b.classList.remove('active'));
          btn.classList.add('active');
          fetch('/set/color/' + r + '/' + g + '/' + b);
        }
      </script>
    </body>
    </html>
  )rawliteral";
  return html;
}

// Route Handlers
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

void handleStatus() {
  // JSON Status zurückgeben
  String json = "{";
  json += "\"lightsOn\":" + String(isLightOn() ? "true" : "false") + ",";
  json += "\"brightness\":" + String(currentBrightness) + ",";
  json += "\"maxBrightness\":" + String(MAX_BRIGHTNESS) + ",";
  json += "\"r\":" + String(targetColor.r) + ",";
  json += "\"g\":" + String(targetColor.g) + ",";
  json += "\"b\":" + String(targetColor.b) + ",";
  json += "\"timeout\":" + String(TIMEOUT / 1000) + ",";
  json += "\"pir1\":" + String(digitalRead(13)) + ",";
  json += "\"pir2\":" + String(digitalRead(14)) + ",";
  json += "\"status\":\"" + currentStatus + "\",";
  json += "\"error\":\"" + currentError + "\"";
  json += "}";
  
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(200, "application/json", json);
}

// Hilfsfunktion: Extrahiert einen Teil der URI nach dem Prefix
String getUriPart(const String& uri, int partIndex) {
  int start = 0;
  int partCount = 0;

  for (int i = 0; i < uri.length(); i++) {
    if (uri[i] == '/') {
      if (partCount == partIndex && i > start) {
        return uri.substring(start, i);
      }
      start = i + 1;
      partCount++;
    }
  }

  // Letzter Teil (kein trailing /)
  if (partCount == partIndex) {
    return uri.substring(start);
  }

  return "";
}

// Handler für alle /set/... Routen
void handleNotFound() {
  String uri = server.uri();

  // /set/brightness/<value>
  if (uri.startsWith("/set/brightness/")) {
    String valStr = uri.substring(16); // Nach "/set/brightness/"
    int val = valStr.toInt();
    if (val >= 0 && val <= 255) {
      setMaxBrightness(val);
      updateStatus("Helligkeit auf " + String(val) + " gesetzt");
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Ungültiger Wert (0-255)");
    }
    return;
  }

  // /set/timeout/<value>
  if (uri.startsWith("/set/timeout/")) {
    String valStr = uri.substring(13); // Nach "/set/timeout/"
    int val = valStr.toInt();
    if (val >= 1 && val <= 300) {
      setTimeoutValue(val * 1000);
      updateStatus("Timeout auf " + String(val) + "s gesetzt");
      server.send(200, "text/plain", "OK");
    } else {
      server.send(400, "text/plain", "Ungültiger Wert (1-300)");
    }
    return;
  }

  // /set/color/<r>/<g>/<b>
  if (uri.startsWith("/set/color/")) {
    // URI Format: /set/color/255/140/60
    String colorPart = uri.substring(11); // Nach "/set/color/"
    int firstSlash = colorPart.indexOf('/');
    int secondSlash = colorPart.indexOf('/', firstSlash + 1);

    if (firstSlash > 0 && secondSlash > firstSlash) {
      int r = colorPart.substring(0, firstSlash).toInt();
      int g = colorPart.substring(firstSlash + 1, secondSlash).toInt();
      int b = colorPart.substring(secondSlash + 1).toInt();

      if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
        setColor(r, g, b);
        updateStatus("Farbe auf RGB(" + String(r) + "," + String(g) + "," + String(b) + ") gesetzt");
        server.send(200, "text/plain", "OK");
      } else {
        server.send(400, "text/plain", "Ungültige RGB-Werte (0-255)");
      }
    } else {
      server.send(400, "text/plain", "Format: /set/color/r/g/b");
    }
    return;
  }

  // Keine bekannte Route
  server.send(404, "text/plain", "Nicht gefunden: " + uri);
}

void setupWebserver() {
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);  // Fängt alle anderen Routen ab

  server.begin();
  Serial.println("Webserver gestartet auf Port 80!");
  Serial.println("Öffne: http://192.168.0.216");
}

void handleWebserver() {
  server.handleClient();
}