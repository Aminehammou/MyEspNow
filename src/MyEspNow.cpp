#include "MyEspNow.h"
#include <Arduino.h>

// Définition du membre statique
EspNowDataReceivedCallback MyEspNow::onDataReceived = nullptr;
volatile bool MyEspNow::appAckReceived = false;
int MyEspNow::waitingForAckId = -1;

MyEspNow::MyEspNow() {}

bool MyEspNow::begin() {
    // Mettre l'appareil en mode Station Wi-Fi
    WiFi.mode(WIFI_STA);
    Serial.print("Adresse MAC de cet ESP32 : ");
    Serial.println(WiFi.macAddress());

    // Initialiser ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Erreur lors de l'initialisation d'ESP-NOW");
        return false;
    }

    // Enregistrer les callbacks
    esp_now_register_send_cb(MyEspNow::onDataSent);
    esp_now_register_recv_cb(MyEspNow::onDataRecv);

    return true;
}

void MyEspNow::setOnDataReceivedCallback(EspNowDataReceivedCallback callback) {
    onDataReceived = callback;
}

bool MyEspNow::addPeer(const uint8_t* peer_addr) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peer_addr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    // Ajouter le pair
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Échec de l'ajout du pair");
        return false;
    }
    return true;
}

bool MyEspNow::sendData(const uint8_t* peer_addr, const MyEspNowData& data) {
    esp_err_t result = esp_now_send(peer_addr, (const uint8_t*)&data, sizeof(data));
    return result == ESP_OK;
}

// Nouvelle fonction pour un envoi fiable avec accusé de réception et tentatives
bool MyEspNow::sendWithAck(const uint8_t* peer_addr, MyEspNowData& data, int retries, int ack_timeout_ms) {
    static int messageId = 0;
    data.id = messageId++; // Assigner un ID unique au message

    waitingForAckId = data.id;

    for (int i = 0; i < retries; i++) {
        appAckReceived = false;

        // Envoyer les données
        esp_err_t result = esp_now_send(peer_addr, (const uint8_t*)&data, sizeof(data));

        if (result == ESP_OK) {
            Serial.printf("Tentative %d/%d: Envoi du message ID %d...\n", i + 1, retries, data.id);
        } else {
            Serial.printf("Tentative %d/%d: Échec de l'envoi du message ID %d.\n", i + 1, retries, data.id);
            delay(100); // Attendre un peu avant de réessayer
            continue;
        }

        // Attendre l'ACK applicatif depuis onDataRecv
        unsigned long ack_wait_start = millis();
        while (!appAckReceived && (millis() - ack_wait_start < ack_timeout_ms)) {
            delay(1); // Laisser d'autres tâches s'exécuter, y compris la réception de l'ACK
        }

        if (appAckReceived) {
            Serial.printf("ACK applicatif reçu pour l'ID %d.\n", data.id);
            waitingForAckId = -1; // Réinitialiser
            return true;
        } else {
            Serial.printf("Tentative %d/%d: Timeout de l'ACK applicatif. Nouvelle tentative...\n", i + 1, retries);
        }
    }

    Serial.printf("Échec final de l'envoi du message ID %d après %d tentatives.\n", data.id, retries);
    waitingForAckId = -1; // Réinitialiser
    return false;
}

void MyEspNow::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ACK MAC: Envoi réussi" : "ACK MAC: Échec de l'envoi");
}

void MyEspNow::onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
    if (len <= sizeof(MyEspNowData)) {
        MyEspNowData receivedData;
        memcpy(&receivedData, incomingData, len);

        // Vérifier si c'est un message ACK et s'il correspond à l'ID attendu
        if (receivedData.cmd == CMD_ACK && receivedData.id == waitingForAckId) {
            appAckReceived = true;
        }

        // Toujours transmettre le message à la fonction de rappel de l'application principale
        if (onDataReceived != nullptr) {
            onDataReceived(mac_addr, receivedData);
        }
    }
}