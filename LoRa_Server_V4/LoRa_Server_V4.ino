#include <WiFi.h>
#include <Firebase.h>
#include <SPI.h>
#include <LoRa.h>
#include "secrets.h"

// --- กำหนดค่า LoRa ---
const int LORA_CS = 5;
const int LORA_RST = 14;
const int LORA_IRQ = 2;
const long LORA_FREQUENCY = 433E6;
const long LORA_BANDWIDTH = 125E3;
const byte LORA_SPREADINGFACTOR = 7;
const byte LORA_CODINGRATE = 5;

// --- ตัวแปร Firebase ---
Firebase fb(REFERENCE_URL);
// FirebaseJson json;
unsigned long lastFirebaseCheck = 0;
const unsigned long firebaseInterval = 3000;  // ตรวจสอบ Firebase ทุก 3 วินาที

void setup() {
  Serial.begin(115200);
  Serial.println("ESP32 LoRa Firebase Server");

  // --- เชื่อมต่อ Wi-Fi ---
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println("\nWiFi Connected!");

  // --- เริ่มต้น Firebase ---
  Serial.println("Firebase Connected!");

  // fb.setString("client1/state", "active");
  // fb.setString("client2/state", "active");
  // --- เริ่มต้น LoRa ---
  SPI.begin();
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  Serial.println("Attempting LoRa.begin()");
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa init failed!");
    while (1)
      ;
  }
  Serial.println("LoRa init OK!");
  Serial.println(fb.getString("client1/state"));
  Serial.println(fb.getString("client2/state"));
}

void loop() {
  // --- รับข้อมูลจาก Client LoRa ---
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedPacket = "";
    while (LoRa.available()) {
      receivedPacket += (char)LoRa.read();
    }
    int rssi = LoRa.packetRssi();
    Serial.print("Received LoRa packet '");
    Serial.print(receivedPacket);
    Serial.print("' with RSSI ");
    Serial.println(rssi);

    int firstSeparator = receivedPacket.indexOf(':');
    Serial.println(firstSeparator);
    if (firstSeparator != -1) {
      String clientID = receivedPacket.substring(7, firstSeparator + 9);
      Serial.println(clientID);
      String remainingData = receivedPacket.substring(firstSeparator + 10);
      Serial.println(remainingData);

      if (remainingData) {
        // String gpsData = remainingData.substring(4);  // ตัด "GPS:" ออก
        String gpsData = remainingData;  // ตัด "GPS:" ออก

        Serial.println(gpsData);
        int commaIndex = gpsData.indexOf(',');
        if (commaIndex != -1) {
          String latitudeStr = gpsData.substring(0, commaIndex);
          String longitudeStr = gpsData.substring(commaIndex + 1);

          float latitude = latitudeStr.toFloat();
          float longitude = longitudeStr.toFloat();

          String latPath = clientID + "/lat";
          String lonPath = clientID + "/lon";

          if (fb.setFloat(latPath, latitude) && fb.setFloat(lonPath, longitude)) {
            Serial.printf("Uploaded GPS to Firebase: %s = %f, %s = %f\n", latPath.c_str(), latitude, lonPath.c_str(), longitude);
          } else {
            Serial.printf("Firebase Error (Set GPS): %s\n");
          }
        } else {
          Serial.println("Error: Invalid GPS data format (missing comma).");
        }
      }
    } else {
      Serial.println("Error: Invalid packet format (missing first colon).");
    }
  }

  // --- ตรวจสอบ Firebase และส่งสถานะไปยัง Client ---
  // if (millis() - lastFirebaseCheck > firebaseInterval) {
  //   lastFirebaseCheck = millis();

    // ดึงสถานะ Client 01
    String stateClient01 = fb.getString("Client01/state");

    if (stateClient01 == "active") {
      sendLoRaPacket("Client01", "ACTIVE");
      Serial.println("Sent ACTIVE to Client01");
    }

    // ดึงสถานะ Client 02
    String stateClient02 = fb.getString("Client02/state");

    if (stateClient02 == "active") {
      sendLoRaPacket("Client02", "ACTIVE");
      Serial.println("Sent ACTIVE to Client02");
    }
  // }

  delay(300);
}

void sendLoRaPacket(String destinationID, String message) {
  LoRa.beginPacket();
  LoRa.print(destinationID);
  LoRa.print(":");
  LoRa.print(message);
  LoRa.endPacket();
  Serial.printf("Sent LoRa packet to %s: %s\n", destinationID.c_str(), message.c_str());
}