#ifndef MY_ESP_NOW_H
#define MY_ESP_NOW_H

#include <esp_now.h>
#include <WiFi.h>
#include <functional>

// Énumération pour identifier le type de message
enum CommandType {
    CMD_CHANGE_PAGE,
    CMD_SENSOR_DATA,
    CMD_ACK // Accusé de réception
};

// Structure pour l'échange de données.
// Assurez-vous que la taille totale ne dépasse pas 250 octets.
struct MyEspNowData {
    CommandType cmd;
    int id;
    float value1;
    float value2;
    char text[100]; // Gardez un champ texte pour la flexibilité
};

// Type de fonction de rappel pour les données reçues
using EspNowDataReceivedCallback = std::function<void(const uint8_t* mac_addr, const MyEspNowData& data)>;

class MyEspNow {
public:
    MyEspNow();
    bool begin();
    void setOnDataReceivedCallback(EspNowDataReceivedCallback callback);
    bool sendData(const uint8_t* peer_addr, const MyEspNowData& data);
    // Nouvelle fonction pour un envoi fiable avec accusé de réception et tentatives
    bool sendWithAck(const uint8_t* peer_addr, MyEspNowData& data, int retries = 5, int ack_timeout_ms = 200);
    bool addPeer(const uint8_t* peer_addr);
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);

private:
    static EspNowDataReceivedCallback onDataReceived;
    // Variables pour gérer le mécanisme d'ACK
    static volatile bool appAckReceived;
    static int waitingForAckId;
};

#endif // MY_ESP_NOW_H