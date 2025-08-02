// =================================================================
// ============== CODE POUR LE RÉCEPTEUR (AFFICHEUR) ===============
// =================================================================
#include <Arduino.h>
#include "MyEspNow.h"
#include "OledManager.h" // On inclut notre bibliothèque d'affichage

// --- Configuration ---
// Remplacez par l'adresse MAC de votre ÉMETTEUR
uint8_t emetteurAddress[] = {0xCC, 0xDB, 0xA7, 0x5B, 0x99, 0x0C};

// --- Objets globaux ---
MyEspNow espnow;
OledManager oled;

// --- Structure de données personnalisée pour le nouveau système de paquets ---
struct SensorDataPacket {
    float temperature;
    float humidity;
    int pressure;
    char description[50];
};

// --- Fonctions de Callback ---

// Callback pour l'ancien format de données (MyEspNowData)
void onDataReceived(const uint8_t* mac_addr, const MyEspNowData& data) {
    Serial.println("
--- Message (Ancien Format) Reçu ---");
    Serial.printf("ID: %d, Valeur1: %.2f, Valeur2: %.2f, Texte: %s
", data.id, data.value1, data.value2, data.text);

    if (data.cmd == CMD_SENSOR_DATA) {
        oled.clearBuffer();
        oled.drawText(0, 0, "Ancien Format");
        oled.drawText(0, 16, "Temp: " + String(data.value1) + " C");
        oled.drawText(0, 32, "Hum:  " + String(data.value2) + " %");
        oled.display();

        // Renvoyer un ACK
        MyEspNowData ackData;
        ackData.cmd = CMD_ACK;
        ackData.id = data.id;
        espnow.sendData(emetteurAddress, ackData);
    }
}

// Callback pour le nouveau format de paquet générique
void onPacketReceived(const uint8_t* mac_addr, const uint8_t* data, uint8_t len) {
    Serial.println("
--- Paquet (Nouveau Format) Reçu ---");
    if (len == sizeof(SensorDataPacket)) {
        SensorDataPacket packet;
        memcpy(&packet, data, len);

        Serial.printf("Temp: %.2f, Hum: %.2f, Pres: %d
", packet.temperature, packet.humidity, packet.pressure);
        Serial.printf("Desc: %s
", packet.description);

        oled.clearBuffer();
        oled.drawText(0, 0, "Nouveau Format");
        oled.drawText(0, 16, "Temp: " + String(packet.temperature, 1) + "C");
        oled.drawText(0, 32, "Hum:  " + String(packet.humidity, 1) + "%");
        oled.drawText(0, 48, "Pres: " + String(packet.pressure) + "hPa");
        oled.display();
    }
}

void setup() {
    Serial.begin(115200);
    
    if (!oled.begin()) {
        Serial.println("Erreur OLED");
        while(true);
    }
    oled.clearBuffer();
    oled.drawCenteredText(28, "En attente...");
    oled.display();

    if (!espnow.begin()) {
        Serial.println("Erreur ESP-NOW");
        while(true);
    }

    espnow.addPeer(emetteurAddress);

    // Enregistrer les deux fonctions de callback
    espnow.setOnDataReceivedCallback(onDataReceived);
    espnow.setOnPacketReceivedCallback(onPacketReceived);
}

void loop() {
    // Le loop peut rester vide.
}
