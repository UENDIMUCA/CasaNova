#include <ArduinoBLE.h>
#include <Arduino_LSM6DS3.h>

#define SERVICE_UUID "00001010-0000-1000-8000-00805f9b34fb"
#define CHARACTERISTIC_UUID "DC74752C-9775-4EA0-9EE6-E574504EF47D"

BLEService myService(SERVICE_UUID);
BLEStringCharacteristic myCharacteristic(CHARACTERISTIC_UUID, BLERead | BLENotify, 50); // Max 50 caractères
float sensitivity = 1.5;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  if (!IMU.begin()) {
    Serial.println("cannot init LSM6DS3 !");
    while (1);
  }
  Serial.println("sensor LSM6DS3 init.");

  if (!BLE.begin()) {
    Serial.println("Starting impossible: BLE !");
    while (1);
  }

  BLE.setLocalName("Nano33IOT");
  BLE.setAdvertisedService(myService);

  myService.addCharacteristic(myCharacteristic);

  BLE.addService(myService);

  // Commencer à diffuser
  BLE.advertise();
  Serial.println("BLE server is brodcasting...");
}

void loop() {
  BLEDevice central = BLE.central();

  if (central) {
    Serial.print("Connecté à : ");
    Serial.println(central.address());

    while (central.connected()) {
      float x, y, z;

      if (IMU.accelerationAvailable()) {
        IMU.readAcceleration(x, y, z);
        
        if (abs(x) > sensitivity || abs(y) > sensitivity || abs(z) > sensitivity) { 
          String message = "Moving";
          myCharacteristic.writeValue(message); 
          Serial.println(message);
          delay(1000); 
        }
      }
    }

    Serial.println("Central disconnected.");
  }
}
