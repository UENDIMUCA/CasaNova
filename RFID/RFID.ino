#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>


#define RST_PIN 22    // Pin RST for RC522
#define SDA_PIN 21    // Pin SDA for RC522

#define BUZZER_PIN 5  // Pin for buzzer
#define LED_RED_PIN 4 // Pin for red LED
#define LED_GREEN_PIN 2 // Pin for green LED

const char* ssid = "Wendy iPhone";
const char* password = "1234567890";
const char* serverName = "http://raspberrypi.local:1880/api/esp32/rfid";


MFRC522 mfrc522(SDA_PIN, RST_PIN);  // Create MFRC522 instance

void setup() {
  Serial.begin(115200);   // Initialize serial communication
  
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Connected to WiFi");
  
  SPI.begin();            // Initialize SPI bus
  mfrc522.PCD_Init();     // Initialize RC522 module
  Serial.println("RFID ready.");

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  digitalWrite(LED_RED_PIN, HIGH);
}

void loop() {
  // Check for new card presence
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Select the card
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Convert UID to hexadecimal string
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) {
      uid += "0";  // Add leading zero for single hex digit
    }
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();  // Convert to uppercase for consistency

  // Display UID
  Serial.print("UID de la carte : ");
  Serial.println(uid);
  
  // Trigger buzzer and LEDs
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, HIGH);

  sendUID(uid);

  mfrc522.PICC_HaltA();
}
void sendUID(String uid) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.begin(serverName);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    String httpRequestData = "uid=" + uid;
    int httpResponseCode = http.POST(httpRequestData);
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.println(httpResponseCode);
      Serial.println(response);
    } else {
      Serial.print("Error on sending POST: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi Disconnected");
  }
}
