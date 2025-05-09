#include <SPI.h>
#include <LoRa.h>
#include <TinyGPS++.h>
#include <HardwareSerial.h>

#define RXPin (16)
#define TXPin (17)

// กำหนดหมายเลขขา LoRa
const int LORA_CS = 5;
const int LORA_RST = 14;
const int LORA_IRQ = 2;
const long LORA_FREQUENCY = 433E6;
const long LORA_BANDWIDTH = 125E3;
const byte LORA_SPREADINGFACTOR = 7;
const byte LORA_CODINGRATE = 5;

// กำหนด ID ของ Client ตัวเอง (ต้องไม่ซ้ำกัน)
const String CLIENT_ID = "Client02";  // หรือ "Client02" สำหรับอีกตัว

int button = 12;
int buzzer = 15;
String messageToSend = "Hello from " + CLIENT_ID;
long lastSendTime = 0;
const long sendInterval = 5000;

static const uint32_t GPSBaud = 9600;
TinyGPSPlus gps;
HardwareSerial ss(2);

void setup() {
  Serial.begin(115200);
  pinMode(buzzer, OUTPUT);
  pinMode(button, INPUT_PULLUP);
  Serial.println(CLIENT_ID + " LoRa Client");
  ss.begin(GPSBaud, SERIAL_8N1, RXPin, TXPin, false);
  SPI.begin();
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);
  Serial.println("Attempting LoRa.begin()");
  if (!LoRa.begin(LORA_FREQUENCY)) {
    Serial.println("LoRa init failed!");
    while (1)
      ;
  }
  Serial.println("LoRa init OK!");
  digitalWrite(buzzer, 1);
  delay(1000);
  digitalWrite(buzzer, 0);

  // ... (ตั้งค่า LoRa เหมือนเดิม) ...
}

void loop() {

  while (ss.available() > 0)
    if (gps.encode(ss.read()))
      displayInfo();

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while (true)
      ;
  }

  // รับข้อมูล
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    String receivedPacket = "";
    while (LoRa.available()) {
      receivedPacket += (char)LoRa.read();
    }
    Serial.print("Received: ");
    Serial.print(receivedPacket);
    int recipientSeparator = receivedPacket.indexOf(':');
    if (recipientSeparator != -1) {
      String recipientID = receivedPacket.substring(0, recipientSeparator);
      if (recipientID == CLIENT_ID || recipientID == "Broadcast") {  // ตรวจสอบว่าเป็นข้อความสำหรับตัวเองหรือ Broadcast
        String message = receivedPacket.substring(recipientSeparator + 1);
        Serial.print(" - Message for me/broadcast: ");
        Serial.println(message);
        // ประมวลผลข้อความที่ได้รับ
        if (message == "ACTIVE") {
          digitalWrite(buzzer, 1);
          delay(1000);
          digitalWrite(buzzer, 0);
        }
      } else {
        Serial.println(" - Not for me.");
      }
    }
    Serial.print(" RSSI: ");
    Serial.println(LoRa.packetRssi());
  }

  delay(10);
}

void sendLoRaPacket(String packet) {
  LoRa.beginPacket();
  LoRa.print(packet);
  LoRa.endPacket();
}

void displayInfo() {

  // ส่งข้อมูลเป็นระยะ
  if (digitalRead(button) == 0) {
    Serial.print(F("Location: "));
    // String packet = "Server:" + CLIENT_ID + ":" + messageToSend;  // ระบุผู้รับเป็น "Server"

    if (gps.location.isValid()) {
      Serial.print(gps.location.lat(), 6);
      Serial.print(F(","));
      Serial.print(gps.location.lng(), 6);
    } else {
      Serial.print(F("INVALID"));
    }
    String packet = "Server:" + CLIENT_ID + ":" + String(gps.location.lat(), 6) + "," + String(gps.location.lng(), 6);
    Serial.print("Sending: ");
    Serial.println(packet);
    sendLoRaPacket(packet);
  }
  Serial.println();
}