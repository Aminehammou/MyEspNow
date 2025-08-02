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

// --- Fonction de Callback ---
// C'est cette fonction qui sera exécutée à chaque fois que des données sont reçues.
void onDataReceived(const uint8_t* mac_addr, const MyEspNowData& data) {
    Serial.print("Message reçu de : ");
    for (int i = 0; i < 6; i++) {
        Serial.printf("%02X", mac_addr[i]);
        if (i < 5) Serial.print(":");
    }
    Serial.printf("\nID du message: %d\n", data.id);

    // On traite le message seulement si c'est une commande de données de capteur
    if (data.cmd == CMD_SENSOR_DATA) {
        Serial.printf("Données reçues -> Valeur1: %.2f, Valeur2: %.2f, Texte: %s\n", data.value1, data.value2, data.text);

        // 1. Afficher les données sur l'écran OLED
        oled.clearBuffer();
        oled.drawText(0, 0, "Donnees Recues");
        oled.drawText(0, 16, "Temp: " + String(data.value1) + " C");
        oled.drawText(0, 32, "Hum:  " + String(data.value2) + " %");
        oled.drawText(0, 48, data.text);
        oled.display();

        // 2. Renvoyer un accusé de réception (ACK) à l'émetteur
        MyEspNowData ackData;
        ackData.cmd = CMD_ACK;
        ackData.id = data.id; // C'est crucial : on renvoie le même ID pour que l'émetteur sache quel message a été reçu.
        
        // On utilise sendData (non-bloquant) pour l'ACK, pas besoin d'attendre un ACK pour un ACK.
        espnow.sendData(emetteurAddress, ackData); 
    }
}

void setup() {
    Serial.begin(115200);
    
    // Initialiser l'écran OLED
    if (!oled.begin()) {
        Serial.println("Erreur OLED");
        while(true);
    }
    oled.clearBuffer();
    oled.drawCenteredText(28, "En attente...");
    oled.display();

    // Initialiser ESP-NOW
    if (!espnow.begin()) {
        Serial.println("Erreur ESP-NOW");
        while(true);
    }

    // Ajouter l'émetteur comme pair
    espnow.addPeer(emetteurAddress);

    // Enregistrer notre fonction de callback
    espnow.setOnDataReceivedCallback(onDataReceived);
}

void loop() {
    // Le loop peut rester vide. La réception des données est gérée par les interruptions.
}
