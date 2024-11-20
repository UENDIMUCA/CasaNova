#include <SPI.h>
#include <MFRC522.h>

#define RST_PIN 22   // Pin RST pour RC522
#define SDA_PIN 21   // Pin SDA pour RC522

#define BUZZER_PIN 5 // Pin pour buzzer
#define LED_RED_PIN 4 // Pin pour led rouge
#define LED_GREEN_PIN 2 // Pin pour led verte

MFRC522 mfrc522(SDA_PIN, RST_PIN);  // Créer une instance de MFRC522


void setup() {
    Serial.begin(115200);   // Initialiser la communication série
  SPI.begin();            // Initialiser le bus SPI
  mfrc522.PCD_Init();     // Initialiser le module RC522
  Serial.println("RFID ready.");

  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(LED_RED_PIN, OUTPUT);
  pinMode(LED_GREEN_PIN, OUTPUT);

  digitalWrite(LED_RED_PIN, HIGH);
}

void loop() {
  // Vérifier la présence d'une nouvelle carte
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }

  // Sélectionner la carte
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }

  // Afficher l'UID de la carte
  Serial.print("UID de la carte : ");
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
  }
  Serial.println();

  digitalWrite(BUZZER_PIN, HIGH);
  digitalWrite(LED_RED_PIN, LOW);
  digitalWrite(LED_GREEN_PIN, HIGH);
  delay(200);
  digitalWrite(BUZZER_PIN, LOW);
  delay(300);
  digitalWrite(LED_GREEN_PIN, LOW);
  digitalWrite(LED_RED_PIN, HIGH);

  mfrc522.PICC_HaltA();

}
