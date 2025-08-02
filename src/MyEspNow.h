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

// Énumération pour le type de message interne, afin de différencier les structures de données
enum MyEspNowMessageType : uint8_t {
    TYPE_LEGACY_DATA = 0x01,
    TYPE_GENERIC_PACKET = 0x02
};

// Type de fonction de rappel pour les données reçues (structure originale)
using EspNowDataReceivedCallback = std::function<void(const uint8_t* mac_addr, const MyEspNowData& data)>;
// Nouveau type de fonction de rappel pour les paquets de données génériques
using EspNowPacketReceivedCallback = std::function<void(const uint8_t* mac_addr, const uint8_t* data, uint8_t len)>;


class MyEspNow {
public:
    MyEspNow();
    bool begin();
    
    // Fonctions pour la structure de données originale
    void setOnDataReceivedCallback(EspNowDataReceivedCallback callback);
    bool sendData(const uint8_t* peer_addr, const MyEspNowData& data);
    
    // Nouvelles fonctions pour l'envoi de données génériques
    void setOnPacketReceivedCallback(EspNowPacketReceivedCallback callback);
    bool sendPacket(const uint8_t* peer_addr, const uint8_t* data, size_t len);

    // Fonction d'envoi fiable (conserve la structure originale pour le moment)
    bool sendWithAck(const uint8_t* peer_addr, MyEspNowData& data, int retries = 5, int ack_timeout_ms = 200);
    
    bool addPeer(const uint8_t* peer_addr);
    
    // Callbacks statiques internes
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);

private:
    static EspNowDataReceivedCallback onDataReceived;
    static EspNowPacketReceivedCallback onPacketReceived; // Callback pour les paquets génériques

    // Variables pour gérer le mécanisme d'ACK
    static volatile bool appAckReceived;
    static int waitingForAckId;
};

#endif // MY_ESP_NOW_H