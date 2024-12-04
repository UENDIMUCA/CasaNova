#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "DHT.h"
#include <PubSubClient.h>
#include <HCSR04.h>
#include <HardwareSerial.h>

// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEScan.h>
// #include <BLEClient.h>
// #include <WebServer.h>

// --- Pin Definitions ---
#define RST_PIN 22    // Pin RST for RC522
#define SDA_PIN 21    // Pin SDA for RC522
#define BUZZER_PIN 5  // Pin for buzzer
#define LED_RED_PIN 4 // Pin for red LED
#define LED_GREEN_PIN 2 // Pin for green LED
#define TEMP_LED_BLUE_PIN 13
#define TEMP_LED_RED_PIN 12

// DHT sensor setup
#define DHTPIN 15        // Pin connected to DHT sensor
#define DHTTYPE DHT11    // DHT 11 sensor

// HC-SR04 Ultrasonic sensor setup
const int TRIG_PIN = 27;
const int ECHO_PIN = 26;

// RGB LED for humidity
#define HUMID_RED_PIN 32
#define HUMID_BLUE_PIN 25

// Thresholds
#define SOUND_SPEED 340 // Speed of sound in air (m/s)
#define MAX_THRESHOLD 28.0
#define MIN_THRESHOLD 26.0
#define MAX_HUMIDITY 60.0  // Maximum humidity threshold
#define MIN_HUMIDITY 40.0  // Minimum humidity threshold

// WiFi configuration
const char ssid[] PROGMEM = "Wendy iPhone";
const char password[] PROGMEM = "1234567890";

// Server and MQTT configuration
const char serverName[] PROGMEM = "http://raspberrypi.local:1880/api/esp32/rfid";
const char mqttServer[] PROGMEM = "172.20.10.10"; // Replace with your Raspberry Pi's IP
const int mqttPort = 1884;
const char mqttTopic[] PROGMEM = "sensor/dht";

// // --- Bluetooth variables ---
// static BLEUUID serviceUUID("00001010-0000-1000-8000-00805f9b34fb");
// static BLEUUID charUUID("DC74752C-9775-4EA0-9EE6-E574504EF47D");

// static BLERemoteCharacteristic* pRemoteCharacteristic;
// static boolean doConnect = false;
// static boolean connected = false;
// static BLEAdvertisedDevice* myDevice;

// --- Objects ---
MFRC522 mfrc522(SDA_PIN, RST_PIN); // RFID module
DHT dht(DHTPIN, DHTTYPE);          // DHT sensor
UltraSonicDistanceSensor hc(TRIG_PIN, ECHO_PIN); // Ultrasonic sensor
WiFiClient espClient;
PubSubClient mqttClient(espClient);
HardwareSerial mySerial(2); // UART2
// WebServer server(8081); // Creating a server

// --- Variables ---
int loopIteration = 0;
int sendInterval = 10;
unsigned long lastRFIDTime = 0;
unsigned long rfidTimeout = 10000; // 10 seconds
bool greenLEDActive = false; // Indique si la LED verte est active


// // --- Bluetooth functions ---
// void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
//   // If motion detected without recent RFID, send UID -1
//   if (millis() - lastRFIDTime > rfidTimeout) {
//     sendUID("-1");
//   }
// }

// class MyClientCallback : public BLEClientCallbacks {
//   void onConnect(BLEClient* pClient) {
//     connected = true;
//   }

//   void onDisconnect(BLEClient* pClient) {
//     connected = false;
//   }
// };

// bool connectToServer() {
//   BLEClient* pClient = BLEDevice::createClient();
//   pClient->setClientCallbacks(new MyClientCallback());
//   pClient->connect(myDevice);
//   BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
//   if (pRemoteService == nullptr) {
//     pClient->disconnect();
//     return false;
//   }

//   pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
//   if (pRemoteCharacteristic == nullptr) {
//     pClient->disconnect();
//     return false;
//   }

//   if (pRemoteCharacteristic->canNotify()) {
//     pRemoteCharacteristic->registerForNotify(notifyCallback);
//   }

//   return true;
// }

// class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
//   void onResult(BLEAdvertisedDevice advertisedDevice) {
//     if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
//       BLEDevice::getScan()->stop();
//       myDevice = new BLEAdvertisedDevice(advertisedDevice);
//       doConnect = true;
//     }
//   }
// };

// --- Setup ---
void setup() {
  Serial.begin(115200);

  mySerial.begin(9600, SERIAL_8N1, 16, 17); // RX2=16, TX2=17 pour ESP32

  Serial.println("ready to had UART2 datas");

  // WiFi setup
  Serial.print("Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("\nConnected to WiFi");
  Serial.println(WiFi.localIP());

  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("RFID ready.");

  dht.begin();
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(TEMP_LED_BLUE_PIN, OUTPUT);
  pinMode(TEMP_LED_RED_PIN, OUTPUT);
  pinMode(HUMID_RED_PIN, OUTPUT);
  pinMode(HUMID_BLUE_PIN, OUTPUT);

  // Set default states
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  mqttClient.setServer(mqttServer, mqttPort);

  // // Server init
  // server.on("/ultrasonic", HTTP_GET, []() {
  //   long distance = hc.measureDistanceCm();
  //   String response;
  //   int httpCode;

  //   Serial.println(distance);

  //   // Check the distance
  //   if (distance > 100 || distance <= 0) { // Not OK
  //       response = "{\"status\":\"Not OK\"}";
  //       httpCode = 400; // HTTP code for a bad request
  //   } else { // OK
  //       response = "{\"status\":\"OK\"}";
  //       httpCode = 200; // HTTP code for a successful request
  //   }

  //   server.send(httpCode, "application/json", response); // Send the response
  // }); 
  // BLEDevice::init("");
  // BLEScan* pBLEScan = BLEDevice::getScan();
  // pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  // pBLEScan->setActiveScan(true);
  // pBLEScan->start(5, false);

    // server.begin();
}

void loop() {
  if (mySerial.available()) {  // Vérifiez si des données sont disponibles sur UART2
    String message = mySerial.readStringUntil('\n'); // Lit le message jusqu'à la fin de la ligne
    Serial.println(message);
    Serial.println("Notification reçue via UART");

    // Réinitialiser l'état vert si une notification est reçue
    if (greenLEDActive) {
      greenLEDActive = false;
      digitalWrite(LED_GREEN_PIN, LOW);
    }else{
      sendUID("-1");
    }
  }

  // Vérifiez si la LED verte doit être désactivée après le timeout
  if (greenLEDActive && (millis() - lastRFIDTime > rfidTimeout)) {
    greenLEDActive = false;
    digitalWrite(LED_GREEN_PIN, LOW);
    Serial.println("Statut vert expiré après 10 secondes");
  }

  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  handleRFID();
  handleSensors();

  delay(500); // Délai pour stabiliser la boucle
}


// --- Handle Sensors ---
void handleSensors() {
  if (loopIteration != 0) {
    return;
  }

  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();

  if (isnan(humidity) || isnan(temperature)) {
    return;
  }

  if (temperature > MAX_THRESHOLD) {
    digitalWrite(TEMP_LED_RED_PIN, HIGH);
    digitalWrite(TEMP_LED_BLUE_PIN, LOW);
  } else if (temperature < MIN_THRESHOLD) {
    digitalWrite(TEMP_LED_BLUE_PIN, HIGH);
    digitalWrite(TEMP_LED_RED_PIN, LOW);
  }else{
    digitalWrite(TEMP_LED_BLUE_PIN, LOW);
    digitalWrite(TEMP_LED_RED_PIN, LOW);
  }

    // Humidity threshold actions
  if (humidity > MAX_HUMIDITY) {
    digitalWrite(HUMID_RED_PIN, HIGH);  // Red LED for high humidity
    digitalWrite(HUMID_BLUE_PIN, LOW); // Turn off blue LED
  } else if (humidity < MIN_HUMIDITY) {
    digitalWrite(HUMID_BLUE_PIN, HIGH); // Blue LED for low humidity
    digitalWrite(HUMID_RED_PIN, LOW);   // Turn off red LED
  } else {
    digitalWrite(HUMID_RED_PIN, LOW);  // Turn off red LED
    digitalWrite(HUMID_BLUE_PIN, LOW); // Turn off blue LED
  }

  String payload = String("{\"temperature\":") + temperature + ",\"humidity\":" + humidity + "}";
  mqttClient.publish(mqttTopic, payload.c_str());
}

// --- Handle RFID ---
void handleRFID() {
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return;
  }

 // Get RFID UID
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    if (mfrc522.uid.uidByte[i] < 0x10) uid += "0";
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();

  lastRFIDTime = millis();

  sendUID(uid);

  mfrc522.PICC_HaltA();
}

void sendUID(String uid) {
  long distance = hc.measureDistanceCm();
  if (distance > 100 || distance < 0) {
    uid = "-1";  // Valeur -1 si la distance est incorrecte
  }

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

      if (httpResponseCode == 404) {
        // Carte non valide : Rouge + buzzer
        triggerAlert(LED_RED_PIN);
      } else {
        // Carte valide : Vert
        greenLEDActive = true;
        digitalWrite(LED_GREEN_PIN, HIGH);
        lastRFIDTime = millis();  // Met à jour le dernier temps RFID
        digitalWrite(BUZZER_PIN, HIGH);
        delay(200);
        digitalWrite(BUZZER_PIN, LOW);

      }
    } else {
      Serial.print("Erreur lors de l'envoi POST : ");
      Serial.println(httpResponseCode);
    }
    http.end();
  } else {
    Serial.println("WiFi déconnecté");
  }
}

void triggerAlert(int ledPin) {
  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(ledPin, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);
  digitalWrite(ledPin, LOW);
}


// --- Reconnect to MQTT ---
void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("Connecting to MQTT...");
    if (mqttClient.connect("ESP32Client")) {
      Serial.println("connected.");
    } else {
      Serial.print("Failed, state: ");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}
