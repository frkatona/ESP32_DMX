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

void setup() {
  // Enable RS-485 driver
  pinMode(DMX_DE_RE_PIN, OUTPUT);
  digitalWrite(DMX_DE_RE_PIN, HIGH);  // driver always enabled for transmit-only

  // Start DMX UART: 250k baud, 8N2, no RX pin (we don't receive)
  DMXSerial.begin(250000, SERIAL_8N2, -1, DMX_TX_PIN);

  // Optional: small delay to let fixtures settle
  delay(500);
}

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

// test proof of concept with constant pan and brightness ramps 

void loop() {
  static int value = 0;
  static int step  = 2;

  // Set channel 1 brightness (0–255)
  dmxData[4] = value; // blue + yellow
  dmxData[0] = value; // pan movement (https://manuals.plus/uking-2/uking-zq02001-25w-moving-head-dj-lights-user-instructions#dmx_addressing)
  dmxData[5] = 110; // master dimmer (?)
  dmxData[6] = value; // pan tilt speed (?)

  // Keep other channels fixed or set them as you like
  // e.g., dmxData[1] = 128; dmxData[2] = 255; ...

  // Send a full DMX frame
  sendDMXFrame(dmxData, DMX_CHANNELS);

  // Fade value up and down
  value += step;
  if (value >= 255) {
    value = 255;
    step  = -step;
  } else if (value <= 0) {
    value = 0;
    step  = -step;
  }

  // Frame rate ~40 Hz (25 ms)
  delay(25);
}
