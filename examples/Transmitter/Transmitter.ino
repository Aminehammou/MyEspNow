// =================================================================
// ================= CODE POUR L'ÉMETTEUR (CAPTEUR) =================
// =================================================================
#include <Arduino.h>
#include "MyEspNow.h"

// --- Configuration ---
// Remplacez par l'adresse MAC de votre RÉCEPTEUR
uint8_t recepteurAddress[] = {0xB8, 0xD6, 0x1A, 0x68, 0xBC, 0x3C};

#define BUTTON_PIN 34 // Broche à laquelle le bouton est connecté (l'autre patte à GND)

// --- Objets globaux ---
MyEspNow espnow;

// --- Variables pour l'anti-rebond du bouton ---
unsigned long lastButtonPressTime = 0;
const long debounceDelay = 500; // 500 ms d'anti-rebond

void setup() {
    Serial.begin(115200);
    
    // Configurer le bouton avec une résistance de pull-up interne
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Initialiser ESP-NOW
    if (!espnow.begin()) {
        Serial.println("Erreur ESP-NOW");
        while(true);
    }

    // Ajouter le récepteur comme pair
    espnow.addPeer(recepteurAddress);
}

// --- Structure de données personnalisée pour le nouveau système de paquets ---
struct SensorDataPacket {
    float temperature;
    float humidity;
    int pressure;
    char description[50];
};

void loop() {
    // Vérifier si le bouton est pressé (LOW car on utilise INPUT_PULLUP)
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Logique d'anti-rebond
        if (millis() - lastButtonPressTime > debounceDelay) {
            Serial.println("Bouton pressé ! Envoi des données...");

            // --- Exemple 1: Envoi avec l'ancienne méthode (sendData) ---
            MyEspNowData legacyData;
            legacyData.cmd = CMD_SENSOR_DATA;
            legacyData.value1 = random(20, 30); // Simule une température
            legacyData.value2 = random(40, 60); // Simule une humidité
            strcpy(legacyData.text, "Ancien Format");

            Serial.println("
Envoi avec l'ancienne méthode (sendWithAck)...");
            if (espnow.sendWithAck(recepteurAddress, legacyData)) {
                Serial.println("Ancien format: Envoi réussi avec ACK !");
            } else {
                Serial.println("Ancien format: Échec de l'envoi.");
            }

            delay(200); // Petite pause entre les envois

            // --- Exemple 2: Envoi avec la nouvelle méthode (sendPacket) ---
            SensorDataPacket packetData;
            packetData.temperature = 25.5 + random(-2, 2);
            packetData.humidity = 55.0 + random(-5, 5);
            packetData.pressure = 1013 + random(-10, 10);
            strcpy(packetData.description, "Nouveau format de paquet flexible");

            Serial.println("
Envoi avec la nouvelle méthode (sendPacket)...");
            if (espnow.sendPacket(recepteurAddress, (const uint8_t*)&packetData, sizeof(packetData))) {
                Serial.println("Nouveau format: Envoi réussi !");
            } else {
                Serial.println("Nouveau format: Échec de l'envoi.");
            }

            // Mettre à jour le temps du dernier appui
            lastButtonPressTime = millis();
        }
    }
}
