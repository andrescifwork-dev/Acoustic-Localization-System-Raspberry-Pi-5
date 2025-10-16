/* NodeLoRa.ino
   Sends JSON payloads with GPS (optional) and triangulation data via LoRa.
   - Uses LoRa library (Sandeep Mistry)
   - Optional TinyGPS++ for GPS
*/

#include <SPI.h>
#include <LoRa.h>
#include <TinyGPSPlus.h>   // optional
//#include <ArduinoJson.h> // optional

// ========== CONFIG ==========
const long LORA_FREQUENCY = 868E6; // change to 915E6 if in US
const int LORA_CS = 10;    // NSS/CS pin
const int LORA_RST = 9;
const int LORA_DIO0 = 2;

const bool USE_GPS = true; // set false if you will send GPS/tri data over Serial
const unsigned long SEND_INTERVAL_MS = 2000; // periodic send interval

const char NODE_ID[] = "NODE01";

// Serial ports
// If using Uno, use SoftwareSerial for GPS (not shown). Example uses Serial1 (Mega, Leonardo, etc.)
#define GPS_SERIAL Serial1   // adjust depending on board; for UNO use SoftwareSerial

// GPS reading objects (optional)
TinyGPSPlus gps;

// State
unsigned long lastSend = 0;

// Helper: build JSON string payload
String buildPayload(double lat, double lon, String triJson, unsigned long ts) {
  // triJson expected like: [{"s":1,"dt":0.12},{"s":2,"dt":0.25}] or empty
  String payload = "{";
  payload += "\"id\":\""; payload += NODE_ID; payload += "\",";
  payload += "\"lat\":"; payload += String(lat, 6); payload += ",";
  payload += "\"lon\":"; payload += String(lon, 6); payload += ",";
  payload += "\"tri\":"; 
  payload += triJson.length() ? triJson : "[]";
  payload += ",";
  payload += "\"ts\":"; payload += String(ts);
  payload += "}";
  return payload;
}

void setup() {
  Serial.begin(115200); // debug / accept commands from Pi
  while (!Serial) {}
  Serial.println("NodeLoRa starting...");

  if (USE_GPS) {
    GPS_SERIAL.begin(9600);
    Serial.println("GPS enabled on GPS_SERIAL");
  }

  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  LoRa.setSyncWord(0x34); // optional: set network sync word
  Serial.println("LoRa initialized");
}

String lastTriJson = "[]"; // tri array JSON from Serial input (if provided)
double lastLat = 0.0, lastLon = 0.0;

void loop() {
  unsigned long now = millis();

  // 1) Read GPS (if enabled)
  if (USE_GPS) {
    while (GPS_SERIAL.available()) {
      char c = GPS_SERIAL.read();
      gps.encode(c);
    }
    if (gps.location.isUpdated()) {
      lastLat = gps.location.lat();
      lastLon = gps.location.lng();
      // debug
      // Serial.print("GPS: "); Serial.print(lastLat,6); Serial.print(","); Serial.println(lastLon,6);
    }
  }

  // 2) Read Serial input from Pi or console for triangulation JSON
  // Expect lines like: TRI:[{"s":1,"dt":0.12},{"s":2,"dt":0.24}]
  if (Serial.available()) {
    String line = Serial.readStringUntil('\n');
    line.trim();
    if (line.startsWith("TRI:")) {
      String triPart = line.substring(4);
      triPart.trim();
      if (triPart.length()) {
        lastTriJson = triPart;
        Serial.print("Updated TRI payload: "); Serial.println(lastTriJson);
      }
    } else if (line == "SEND") {
      // immediate manual send
      sendPacket();
    } else {
      Serial.print("Unknown cmd: "); Serial.println(line);
    }
  }

  // 3) Periodic send
  if (now - lastSend >= SEND_INTERVAL_MS) {
    sendPacket();
    lastSend = now;
  }

  // 4) Listen for incoming LoRa (ACKs) briefly
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String incoming = "";
    while (LoRa.available()) {
      incoming += (char)LoRa.read();
    }
    Serial.print("Received via LoRa: "); Serial.println(incoming);
    // Optional: detect ACK:ID
    if (incoming.startsWith("ACK:")) {
      String ackid = incoming.substring(4);
      if (ackid == NODE_ID) {
        Serial.println("ACK received for this node");
      }
    }
  }
}

// Send current payload via LoRa
void sendPacket() {
  unsigned long ts = (unsigned long) (time(NULL)); // if RTC not available, this will be 0
  String payload = buildPayload(lastLat, lastLon, lastTriJson, ts);
  Serial.print("Sending payload: "); Serial.println(payload);

  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket(true); // true = async (wait for TX to finish internally)
}
