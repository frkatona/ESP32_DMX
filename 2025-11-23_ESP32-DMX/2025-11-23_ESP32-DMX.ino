#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>
#include <LittleFS.h>

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
  Serial.begin(115200);
  
  // Initialize LittleFS
  if(!LittleFS.begin()){
    Serial.println("An Error has occurred while mounting LittleFS");
    return;
  }

  // --- your existing DMX setup ---
  pinMode(DMX_DE_RE_PIN, OUTPUT);
  digitalWrite(DMX_DE_RE_PIN, HIGH);
  DMXSerial.begin(250000, SERIAL_8N2, -1, DMX_TX_PIN);
  delay(500);

  // --- WiFi AP + HTTP server ---
  WiFi.softAP(ssid, pass);

  // Serve static files from LittleFS
  server.serveStatic("/", LittleFS, "/index.html");
  server.serveStatic("/style.css", LittleFS, "/style.css");
  server.serveStatic("/script.js", LittleFS, "/script.js");
  server.serveStatic("/index.html", LittleFS, "/index.html");

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