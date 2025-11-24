#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>

///////////////////////
// Pin configuration //
///////////////////////

// UART2 pins (change if your board uses different pins)
static const int DMX_TX_PIN    = 17;  // ESP32 TX2 -> MAX485 DI
static const int DMX_DE_RE_PIN = 4;   // ESP32 GPIO -> MAX485 DE & /RE tied together

// Number of DMX channels we send (can be up to 512)
static const int DMX_CHANNELS = 9;

HardwareSerial DMXSerial(2);  // UART2

// DMX universe buffer (1-based channels, but we'll store from index 0)
uint8_t dmxData[DMX_CHANNELS] = {0};

/////////////////////////////
// DMX frame send routine  //
/////////////////////////////

void sendDMXFrame(uint8_t *data, int channels) {
  // 1) Generate BREAK + MAB using the TX pin as GPIO

  // Stop UART so we can twiddle the TX pin manually
  DMXSerial.end();

  // Drive TX low for BREAK
  pinMode(DMX_TX_PIN, OUTPUT);
  digitalWrite(DMX_TX_PIN, LOW);
  delayMicroseconds(120);  // BREAK ≥ 88 µs; 120 µs is safe

  // Drive TX high for MAB
  digitalWrite(DMX_TX_PIN, HIGH);
  delayMicroseconds(12);   // MAB ≥ 8 µs; 12 µs is safe

  // 2) Re-start UART on the same pin at 250k 8N2
  DMXSerial.begin(250000, SERIAL_8N2, -1, DMX_TX_PIN);

  // 3) Send start code (0) + channel data
  DMXSerial.write(0x00);  // Start code for normal DMX data

  for (int i = 0; i < channels; i++) {
    DMXSerial.write(data[i]);
  }

  DMXSerial.flush();  // ensure everything has left the UART
}

const char* ssid = "Wireless_DMX_Controller";
const char* pass = "DMX12345";

WebServer server(80);

// If you embed the HTML instead of SPIFFS:
const char indexHtml[] PROGMEM = R"HTML(
<html lang="en">

<head>
  <meta charset="UTF-8" />
  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <title>ESP32 DMX Controller</title>
  <style>
    :root {
      font-family: system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      background: #111;
      color: #eee;
    }

    body {
      margin: 0;
      padding: 1rem;
      display: flex;
      flex-direction: column;
      align-items: center;
      gap: 1rem;
    }

    h1 {
      font-size: 1.5rem;
      margin: 0.5rem 0 0.25rem;
      text-align: center;
    }

    .subtitle {
      font-size: 0.85rem;
      opacity: 0.7;
      text-align: center;
      margin-bottom: 1rem;
    }

    .card {
      background: #1d1d1d;
      border-radius: 12px;
      padding: 1rem;
      max-width: 600px;
      width: 100%;
      box-shadow: 0 8px 20px rgba(0, 0, 0, 0.5);
    }

    .channels {
      display: flex;
      flex-direction: column;
      gap: 0.75rem;
    }

    .channel-row {
      display: grid;
      grid-template-columns: minmax(0, 1fr);
      gap: 0.4rem;
      padding: 0.5rem;
      background: #252525;
      border-radius: 8px;
    }

    @media (min-width: 600px) {
      .channel-row {
        grid-template-columns: 140px minmax(0, 1fr) 60px;
        align-items: center;
      }
    }

    .ch-label {
      font-size: 0.9rem;
      font-weight: 600;
      white-space: nowrap;
      color: #ccc;
    }

    /* Controls */
    input[type="range"] {
      width: 100%;
      cursor: pointer;
    }

    select {
      width: 100%;
      padding: 0.4rem;
      background: #333;
      color: #fff;
      border: 1px solid #444;
      border-radius: 6px;
      font-size: 0.9rem;
    }

    .action-btn {
      width: 100%;
      padding: 0.5rem;
      background: #d32f2f;
      color: white;
      border: none;
      border-radius: 6px;
      font-weight: 600;
      cursor: pointer;
      transition: background 0.2s;
    }

    .action-btn:hover {
      background: #b71c1c;
    }

    .action-btn:active {
      transform: translateY(1px);
    }

    .value-box {
      text-align: right;
      font-variant-numeric: tabular-nums;
      font-size: 0.9rem;
      padding: 0.1rem 0.3rem;
      background: #111;
      border-radius: 4px;
      border: 1px solid #333;
      color: #888;
      min-width: 3ch;
    }

    /* Footer */
    .footer-row {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-top: 1.5rem;
      gap: 0.5rem;
      flex-wrap: wrap;
      border-top: 1px solid #333;
      padding-top: 1rem;
    }

    .bulk-btn {
      border: none;
      border-radius: 999px;
      padding: 0.4rem 0.9rem;
      cursor: pointer;
      font-size: 0.85rem;
      font-weight: 600;
      background: #3a6df0;
      color: white;
      box-shadow: 0 4px 12px rgba(0, 0, 0, 0.4);
    }

    .bulk-btn:active {
      transform: translateY(1px);
      box-shadow: 0 2px 6px rgba(0, 0, 0, 0.4);
    }

    .status {
      font-size: 0.75rem;
      opacity: 0.75;
    }

    .status.ok {
      color: #7dff8b;
    }

    .status.err {
      color: #ff7d7d;
    }
  </style>
</head>

<body>
  <h1>ESP32 DMX Controller</h1>
  <div class="subtitle">Adjust DMX channels 1–9 in real time</div>

  <div class="card">
    <div class="channels" id="channels"></div>

    <div class="footer-row">
      <div>
        <button class="bulk-btn" id="allOffBtn">All Off</button>
        <button class="bulk-btn" id="allFullBtn">All 255</button>
      </div>
      <div class="status" id="status">Idle</div>
    </div>
  </div>

  <script>
    // DMX channels configuration
    const channelDefs = [
      { ch: 1, label: "Yaw", type: "slider" },
      { ch: 2, label: "Tilt", type: "slider" },
      {
        ch: 3,
        label: "Color Wheel",
        type: "select",
        options: [
          { label: "Gobo / Default", value: 0 },
          { label: "Static Pair", value: 72 },
          { label: "Auto", value: 140 }
        ]
      },
      {
        ch: 4,
        label: "Gobo Wheel",
        type: "select",
        options: [
          { label: "Open White", value: 0 },
          { label: "Gobo 1", value: 8 },
          { label: "Gobo 2", value: 16 },
          { label: "Gobo 3", value: 24 },
          { label: "Gobo 4", value: 32 },
          { label: "Gobo 5", value: 40 },
          { label: "Gobo 6", value: 48 },
          { label: "Gobo 7", value: 56 },
          { label: "White Shake", value: 64 },
          { label: "Shake 1", value: 72 },
          { label: "Shake 2", value: 80 },
          { label: "Shake 3", value: 88 },
          { label: "Shake 4", value: 96 },
          { label: "Shake 5", value: 104 },
          { label: "Shake 6", value: 112 },
          { label: "Shake 7", value: 120 },
          { label: "Auto Gobo", value: 128 }
        ]
      },
      { ch: 5, label: "Strobe", type: "slider" },
      { ch: 6, label: "Master Dimmer", type: "slider" },
      { ch: 7, label: "Pan/Tilt Speed", type: "slider" },
      {
        ch: 8,
        label: "Auto Mode",
        type: "select",
        options: [
          { label: "Inactive", value: 0 },
          { label: "Auto", value: 135 },
          { label: "Sound Mode 3", value: 160 },
          { label: "Sound Mode 2", value: 185 },
          { label: "Sound Mode 1", value: 210 },
          { label: "Sound Mode 0", value: 235 }
        ]
      },
      {
        ch: 9,
        label: "Reset",
        type: "button",
        buttonLabel: "RESET DEVICE",
        value: 255
      },
    ];

    const channelsContainer = document.getElementById("channels");
    const statusEl = document.getElementById("status");
    const allOffBtn = document.getElementById("allOffBtn");
    const allFullBtn = document.getElementById("allFullBtn");

    function setStatus(text, ok = true) {
      statusEl.textContent = text;
      statusEl.classList.toggle("ok", ok);
      statusEl.classList.toggle("err", !ok);
    }

    function sendDMXValue(ch, value) {
      // Basic GET /set?ch=<ch>&val=<value>
      fetch(`/set?ch=${encodeURIComponent(ch)}&val=${encodeURIComponent(value)}`)
        .then((res) => {
          if (!res.ok) throw new Error(res.statusText);
          setStatus(`Set ch ${ch} → ${value}`, true);
        })
        .catch((err) => {
          console.error(err);
          setStatus(`Error sending ch ${ch}`, false);
        });
    }

    function createChannelRow(def) {
      const row = document.createElement("div");
      row.className = "channel-row";

      const label = document.createElement("div");
      label.className = "ch-label";
      label.textContent = `Ch ${def.ch} — ${def.label}`;
      row.appendChild(label);

      let control;
      const valueBox = document.createElement("div");
      valueBox.className = "value-box";
      valueBox.textContent = "0";

      if (def.type === "select") {
        control = document.createElement("select");
        def.options.forEach(opt => {
          const option = document.createElement("option");
          option.value = opt.value;
          option.textContent = opt.label;
          control.appendChild(option);
        });

        control.addEventListener("change", () => {
          const v = Number(control.value);
          valueBox.textContent = v;
          sendDMXValue(def.ch, v);
        });

      } else if (def.type === "button") {
        control = document.createElement("button");
        control.className = "action-btn";
        control.textContent = def.buttonLabel || "Trigger";

        control.addEventListener("click", () => {
          const v = def.value || 255;
          valueBox.textContent = v;
          sendDMXValue(def.ch, v);
          // Optional: Reset visual feedback after a moment if it's a momentary action
          setTimeout(() => valueBox.textContent = "-", 1000);
        });
        valueBox.textContent = "-"; // Default state for button

      } else {
        // Default to slider
        control = document.createElement("input");
        control.type = "range";
        control.min = "0";
        control.max = "255";
        control.value = "0";
        control.step = "1";

        control.addEventListener("input", () => {
          valueBox.textContent = control.value;
        });

        control.addEventListener("change", () => {
          const v = Number(control.value) || 0;
          sendDMXValue(def.ch, v);
        });
      }

      row.appendChild(control);
      row.appendChild(valueBox);

      // Store references for bulk updates
      def.control = control;
      def.valueBox = valueBox;

      return row;
    }

    // Build UI
    channelDefs.forEach((def) => {
      const row = createChannelRow(def);
      channelsContainer.appendChild(row);
    });

    // Bulk controls
    allOffBtn.addEventListener("click", () => {
      channelDefs.forEach((def) => {
        if (def.type === "slider") {
          def.control.value = 0;
          def.valueBox.textContent = "0";
          sendDMXValue(def.ch, 0);
        } else if (def.type === "select") {
          // Try to select the first option (usually 0/Inactive)
          def.control.selectedIndex = 0;
          const v = Number(def.control.value);
          def.valueBox.textContent = v;
          sendDMXValue(def.ch, v);
        }
        // Skip buttons for bulk off
      });
    });

    allFullBtn.addEventListener("click", () => {
      channelDefs.forEach((def) => {
        if (def.type === "slider") {
          def.control.value = 255;
          def.valueBox.textContent = "255";
          sendDMXValue(def.ch, 255);
        }
        // Skip selects and buttons for bulk full (unsafe/unpredictable)
      });
    });

    // Optional: sync UI from /state endpoint
    function fetchState() {
      fetch("/state")
        .then((res) => res.json())
        .then((json) => {
          channelDefs.forEach((def) => {
            const v = Number(json[def.ch]) || 0;
            if (def.type === "slider") {
              def.control.value = v;
            } else if (def.type === "select") {
              // Find closest option or exact match
              // Simple exact match for now
              def.control.value = v;
            }
            if (def.valueBox) def.valueBox.textContent = v;
          });
          setStatus("Synced from ESP32", true);
        })
        .catch(() => {
          // It's okay if /state isn't implemented
        });
    }

    setTimeout(fetchState, 1000);
  </script>
</body>

</html>
)HTML";

void handleRoot() {
  server.send_P(200, "text/html", indexHtml);
}

void handleSet() {
  if (!server.hasArg("ch") || !server.hasArg("val")) {
    server.send(400, "text/plain", "Missing ch or val");
    return;
  }

  int ch = server.arg("ch").toInt();
  int val = server.arg("val").toInt();

  if (ch < 1 || ch > DMX_CHANNELS || val < 0 || val > 255) {
    server.send(400, "text/plain", "Bad ch/val");
    return;
  }

  dmxData[ch - 1] = (uint8_t)val;
  server.send(200, "text/plain", "OK");
}

void handleState() {
  String json = "{";
  for (int i = 0; i < DMX_CHANNELS; i++) {
    json += "\"" + String(i + 1) + "\":" + String(dmxData[i]);
    if (i < DMX_CHANNELS - 1) json += ",";
  }
  json += "}";
  server.send(200, "application/json", json);
}

void setup() {
  // --- your existing DMX setup ---
  pinMode(DMX_DE_RE_PIN, OUTPUT);
  digitalWrite(DMX_DE_RE_PIN, HIGH);
  DMXSerial.begin(250000, SERIAL_8N2, -1, DMX_TX_PIN);
  delay(500);

  // --- WiFi AP + HTTP server ---
  WiFi.softAP(ssid, pass);

  server.on("/", handleRoot);
  server.on("/set", HTTP_GET, handleSet);
  server.on("/state", HTTP_GET, handleState);
  server.begin();
}

void loop() {
  // Serve HTTP
  server.handleClient();

  // Continuously send current DMX frame
  sendDMXFrame(dmxData, DMX_CHANNELS);
  delay(25);  // ~40 Hz
}