/* MasterLoRa.ino
   Receives LoRa JSON packets, prints them on Serial, and replies with ACK:<nodeid>
*/

#include <SPI.h>
#include <LoRa.h>

const long LORA_FREQUENCY = 868E6; // match node
const int LORA_CS = 10;
const int LORA_RST = 9;
const int LORA_DIO0 = 2;

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println("MasterLoRa starting...");

  LoRa.setPins(LORA_CS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa init failed!");
    while (1);
  }
  LoRa.setSyncWord(0x34);
  Serial.println("LoRa initialized and listening...");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String packet = "";
    while (LoRa.available()) {
      packet += (char)LoRa.read();
    }
    unsigned long rssi = LoRa.packetRssi();
    int snr = LoRa.packetSnr();

    Serial.println("=== RX Packet ===");
    Serial.print("RSSI: "); Serial.println(rssi);
    Serial.print("SNR:  "); Serial.println(snr);
    Serial.print("Payload: "); Serial.println(packet);
    Serial.println("=================");

    // Try extract node id quickly from JSON (simple string search)
    String nodeId = extractNodeId(packet);
    if (nodeId.length()) {
      // Send ACK back
      String ack = "ACK:" + nodeId;
      LoRa.beginPacket();
      LoRa.print(ack);
      LoRa.endPacket();
      Serial.print("ACK sent: "); Serial.println(ack);
    }

    // Optionally forward full payload to PC over Serial for master processing
    Serial.print("FORWARD:"); Serial.println(packet); // master software can read this
  }
  // small sleep to prevent hogging CPU
  delay(10);
}

// Basic extraction of "id":"NODE01" from payload; not full JSON parsing
String extractNodeId(const String &payload) {
  int idx = payload.indexOf("\"id\"");
  if (idx == -1) return "";
  int colon = payload.indexOf(':', idx);
  if (colon == -1) return "";
  int q1 = payload.indexOf('"', colon);
  if (q1 == -1) return "";
  int q2 = payload.indexOf('"', q1 + 1);
  if (q2 == -1) return "";
  String id = payload.substring(q1 + 1, q2);
  return id;
}
