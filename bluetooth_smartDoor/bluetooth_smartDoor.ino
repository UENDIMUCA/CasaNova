#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>

// Utiliser Serial2 pour les broches RX2 et TX2
HardwareSerial mySerial(2); // UART2

// --- Bluetooth variables ---
static BLEUUID serviceUUID("00001010-0000-1000-8000-00805f9b34fb");
static BLEUUID charUUID("DC74752C-9775-4EA0-9EE6-E574504EF47D");

static BLERemoteCharacteristic* pRemoteCharacteristic;
static boolean doConnect = false;
static boolean connected = false;
static BLEAdvertisedDevice* myDevice;

// --- Bluetooth functions ---
void notifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("Notification received: ");
    for (size_t i = 0; i < length; i++) {
        Serial.print((char)pData[i]); // Print the notification data
    }
    Serial.println();
    
    mySerial.println("Alert opening"); // Envoie un message à ESP2
    Serial.println("Message sent to ESP2 via UART2");
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pClient) {
    connected = true;
    Serial.println("Connected to BLE Server");
  }

  void onDisconnect(BLEClient* pClient) {
    connected = false;
    Serial.println("Disconnected from BLE Server");
  }
};

bool connectToServer() {
  Serial.println("Attempting to connect to server...");
  
  BLEClient* pClient = BLEDevice::createClient();
  Serial.println("Client created, setting callbacks");
  pClient->setClientCallbacks(new MyClientCallback());

  if (!pClient->connect(myDevice)) {
    Serial.println("Failed to connect to BLE server");
    return false;
  }

  Serial.println("Connected to server, attempting to get service...");
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    Serial.println("Failed to find service UUID");
    pClient->disconnect();
    return false;
  }

  Serial.println("Service found, attempting to get characteristic...");
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find characteristic UUID");
    pClient->disconnect();
    return false;
  }

  Serial.println("Characteristic found");
  if (pRemoteCharacteristic->canNotify()) {
    Serial.println("Registering for notifications...");
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  } else {
    Serial.println("Characteristic does not support notifications");
  }

  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      Serial.println("Found device with matching service UUID");
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);  // Débogage sur moniteur série via USB
  mySerial.begin(9600, SERIAL_8N1, 16, 17); // RX2=16, TX2=17 pour ESP32
  
  Serial.println("ESP1 ready to send via UART2");

  Serial.println("Initializing BLE...");
  BLEDevice::init("");
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  Serial.println("Starting BLE scan...");
  pBLEScan->start(5, false);
}

void loop() {
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("Connected to server and ready to receive notifications");
    } else {
      Serial.println("Failed to connect to server");
    }
    doConnect = false;
  }

  delay(1000); // Loop delay for stability
}
