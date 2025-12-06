#include <WiFi.h>
#include <WebServer.h>
#include <Arduino.h>
#include <LittleFS.h>
#include <WiFiUdp.h>
#include <DNSServer.h>

#define FORMAT_LITTLEFS_IF_FAILED true


///////////////////////
// Pin configuration //
///////////////////////

// UART2 pins (change if your board uses different pins)
static const int DMX_TX_PIN    = 17;  // ESP32 TX2 -> MAX485 DI
static const int DMX_DE_RE_PIN = 4;   // ESP32 GPIO -> MAX485 DE & /RE tied together

// Number of DMX channels we send (can be up to 512)
static const int DMX_CHANNELS = 512; // Increased to full universe for Art-Net capability
static const int WEB_UI_CHANNELS = 9; // Keep original 9 for Web UI unless expanded later

HardwareSerial DMXSerial(2);  // UART2

// DMX universe buffer (1-based channels, but we'll store from index 0)
uint8_t dmxData[DMX_CHANNELS] = {0};

// Art-Net settings
WiFiUDP Udp;
const int ARTNET_PORT = 6454;
const int ARTNET_BUFFER_MAX = 530; // Header(18) + 512 channels
uint8_t packetBuffer[ARTNET_BUFFER_MAX];

// Captive Portal / Web Server
const byte DNS_PORT = 53;
DNSServer dnsServer;
WebServer server(80);
const char* LOGIN_PIN = "1234";

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

// -- Auth Helper --
bool isCaptivePortal() {
  // If the host requested is NOT our IP, it's a captive portal check
  // (e.g. google.com, captive.apple.com)
  if (!LittleFS.begin(false)) return false; // Safety
  // Just check if host == IP. If not, true.
  // Note: Standard WiFi library 'hostHeader()' is useful.
  // Actually, simplest check: if Host header != 192.168.4.1 (or whatever local IP)
  return (server.hostHeader() != WiFi.softAPIP().toString());
}

bool isAuthenticated() {
  if (server.hasHeader("Cookie")) {
    String cookie = server.header("Cookie");
    if (cookie.indexOf("dmx_auth=1") != -1) return true;
  }
  return false;
}

void handleRoot() {
  if (isCaptivePortal()) {
    // Redirect to login (captive portal detection)
    server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/login.html", true);
    server.send(302, "text/plain", "");
    return;
  }

  if (!isAuthenticated()) {
    server.sendHeader("Location", "/login.html", true);
    server.send(302, "text/plain", "");
    return;
  }

  // If authenticated, serve index
  File file = LittleFS.open("/index.html", "r");
  if (!file) {
    server.send(500, "text/plain", "Index missing");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleLoginUI() {
  if (isCaptivePortal()) {
     server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/login.html", true);
     server.send(302, "text/plain", "");
     return;
  }
  
  if (isAuthenticated()) {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
    return;
  }
  
  File file = LittleFS.open("/login.html", "r");
  if (!file) {
    server.send(500, "text/plain", "Login file missing");
    return;
  }
  server.streamFile(file, "text/html");
  file.close();
}

void handleLoginPost() {
  if (server.hasArg("password") && server.arg("password") == LOGIN_PIN) {
    server.sendHeader("Location", "/", true);
    server.sendHeader("Set-Cookie", "dmx_auth=1; Path=/; Max-Age=3600"); // 1 hour session
    server.send(302, "text/plain", "");
  } else {
    server.sendHeader("Location", "/login.html?error=1", true);
    server.send(302, "text/plain", "");
  }
}

void handleSet() {
  // Protection: Require auth for control?
  // Captive portals often allow some control before auth, but strictly:
  if (!isAuthenticated()) {
     server.send(403, "text/plain", "Forbidden");
     return;
  }

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
  if (!isAuthenticated()) {
     server.send(403, "text/plain", "Forbidden");
     return;
  }
  String json = "{";
  // Only return the first few channels to keep JSON small for now, or up to WEB_UI_CHANNELS
  for (int i = 0; i < WEB_UI_CHANNELS; i++) {
    json += "\"" + String(i + 1) + "\":" + String(dmxData[i]);
    if (i < WEB_UI_CHANNELS - 1) json += ",";
  }
  json += "}";
  server.send(200, "application/json", json);
}

void handleArtNet() {
  int packetSize = Udp.parsePacket();
  if (packetSize && packetSize <= ARTNET_BUFFER_MAX) {
    Udp.read(packetBuffer, ARTNET_BUFFER_MAX);

    // Check header "Art-Net" + 0x00
    if (memcmp(packetBuffer, "Art-Net", 8) == 0) {
      // OpCode (Lo-Hi) for ArtDmx is 0x5000 -> Little Endian 0x00 0x50
      // In packet: OpCode is bytes 8, 9. 
      // OpOutput -> 0x5000.  Byte 8 should be 0x00, Byte 9 should be 0x50.
      if (packetBuffer[8] == 0x00 && packetBuffer[9] == 0x50) {
        // Protocol Version is bytes 10, 11 (should be >= 14)
        // Sequence (12), Physical (13), SubUni (14), Net (15) ignored for simple setup
        // Length (Hi-Lo) at 16, 17
        int dataLength = (packetBuffer[16] << 8) | packetBuffer[17];
        
        // Data starts at byte 18
        if (dataLength > 0 && dataLength <= 512) {
           // Copy data to dmxData
           // Art-Net data is typically 512 bytes max per universe
           // We'll trust the length field but clamp to our buffer size
           int lenToCopy = (dataLength < DMX_CHANNELS) ? dataLength : DMX_CHANNELS;
           memcpy(dmxData, &packetBuffer[18], lenToCopy);
        }
      }
    }
  }
}

void setup() {
  Serial.begin(115200);
  
  // Initialize LittleFS
  // format filesystem if it fails to mount
  if(!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED)){
    Serial.println("An Error has occurred while mounting LittleFS");
  }

  pinMode(DMX_DE_RE_PIN, OUTPUT);
  digitalWrite(DMX_DE_RE_PIN, HIGH);
  DMXSerial.begin(250000, SERIAL_8N2, -1, DMX_TX_PIN);
  delay(500);

  // WiFi AP + HTTP server
  // Note: For Captive Portal to work well on some devices, configure AP with Gateway = IP
  WiFi.softAP(ssid, pass);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(myIP);

  // Start Art-Net UDP
  Udp.begin(ARTNET_PORT);
  Serial.print("Art-Net listening on port ");
  Serial.println(ARTNET_PORT);

  // DNS Server
  dnsServer.start(DNS_PORT, "*", myIP);

  // Web Server Routes
  // Need to collect headers to read Cookies
  const char * headerkeys[] = {"Cookie", "Host"};
  size_t headerkeyssize = sizeof(headerkeys)/sizeof(char*);
  server.collectHeaders(headerkeys, headerkeyssize);

  server.on("/", handleRoot);
  server.on("/login.html", HTTP_GET, handleLoginUI);
  server.on("/login", HTTP_POST, handleLoginPost);
  
  // Directly serve static assets (allow them without auth? no, maybe shield them or allow for login page styles)
  // Actually style.css is needed for login page, so allow it.
  server.serveStatic("/style.css", LittleFS, "/style.css"); 
  // Disable script.js for unauth?
  server.on("/script.js", [](){
      if(!isAuthenticated()) { server.send(403, "text/plain", "Forbidden"); return; }
      File f = LittleFS.open("/script.js", "r");
      server.streamFile(f, "application/javascript");
      f.close();
  });

  server.on("/set", HTTP_GET, handleSet);
  server.on("/state", HTTP_GET, handleState);
  
  // Captive Portal Catch-All
  server.onNotFound([]() {
     if (isCaptivePortal()) {
       server.sendHeader("Location", String("http://") + WiFi.softAPIP().toString() + "/login.html", true);
       server.send(302, "text/plain", "");
     } else {
       // If just a 404 on local IP, maybe redirect to root?
       server.send(404, "text/plain", "Not Found");
     }
  });

  server.begin();
}

void loop() {
  // DNS processing
  dnsServer.processNextRequest();

  // Serve HTTP
  server.handleClient();

  // Check for Art-Net packets
  handleArtNet();

  // Continuously send current DMX frame
  sendDMXFrame(dmxData, DMX_CHANNELS);
  delay(25);  // ~40 Hz
}