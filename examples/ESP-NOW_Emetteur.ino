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

void loop() {
    // Vérifier si le bouton est pressé (LOW car on utilise INPUT_PULLUP)
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Logique d'anti-rebond
        if (millis() - lastButtonPressTime > debounceDelay) {
            Serial.println("Bouton pressé ! Envoi des données...");
            
            // Créer la structure de données à envoyer
            MyEspNowData dataToSend;
            dataToSend.cmd = CMD_SENSOR_DATA;
            dataToSend.value1 = random(20, 30); // Simule une température
            dataToSend.value2 = random(40, 60); // Simule une humidité
            strcpy(dataToSend.text, "Test ESP-NOW");

            // Envoyer les données avec la méthode fiable
            if (espnow.sendWithAck(recepteurAddress, dataToSend)) {
                Serial.println("Envoi réussi avec ACK !");
            } else {
                Serial.println("Échec de l'envoi après plusieurs tentatives.");
            }

            // Mettre à jour le temps du dernier appui
            lastButtonPressTime = millis();
        }
    }
}
