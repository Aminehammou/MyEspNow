# Évolution de la bibliothèque `MyEspNow.h`

Ce document retrace l'évolution du fichier d'en-tête `MyEspNow.h` au cours de notre discussion, depuis une simple communication textuelle jusqu'à un système d'envoi de données structurées et fiables.

## 1. Version initiale : Communication bidirectionnelle simple

La première étape a été de remplacer la communication Bluetooth par ESP-NOW. L'objectif était d'envoyer des commandes textuelles simples entre deux ESP32.

La structure de données était minimale, contenant uniquement un tableau de caractères.

```cpp
// Fichier : MyEspNow.h (Version 1)
#ifndef MY_ESP_NOW_H
#define MY_ESP_NOW_H

#include <esp_now.h>
#include <WiFi.h>
#include <functional>

// Structure pour l'échange de données
// La taille maximale est de 250 octets
struct MyEspNowData {
    char text[250];
};

// Type de fonction de rappel pour les données reçues
using EspNowDataReceivedCallback = std::function<void(const uint8_t* mac_addr, const MyEspNowData& data)>;

class MyEspNow {
public:
    MyEspNow();
    bool begin();
    void setOnDataReceivedCallback(EspNowDataReceivedCallback callback);
    bool sendData(const uint8_t* peer_addr, const MyEspNowData& data);
    bool addPeer(const uint8_t* peer_addr);
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);

private:
    static EspNowDataReceivedCallback onDataReceived;
};

#endif // MY_ESP_NOW_H
```

---

## 2. Deuxième version : Envoi de données structurées

La demande suivante était d'envoyer des données plus complexes, comme des valeurs de capteurs, sans avoir à analyser des chaînes de caractères. Pour cela, la structure `MyEspNowData` a été enrichie.

- Une énumération `CommandType` a été ajoutée pour identifier le type de message.
- La structure de données a été modifiée pour inclure un type de commande, un identifiant, des valeurs flottantes et un champ de texte plus petit.

```cpp
// Fichier : MyEspNow.h (Version 2)
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

// ... (le reste de la classe reste identique)
```

---

## 4. Version Actuelle : Ajout de Paquets de Données Génériques

La dernière amélioration a consisté à ajouter un système de paquets de données génériques pour permettre l'envoi de n'importe quelle structure de données, tout en conservant la compatibilité avec l'ancien système.

- **Double Système de Callback :** La bibliothèque utilise maintenant deux callbacks : un pour la structure `MyEspNowData` originale et un nouveau pour les paquets de données génériques.
- **Type de Message Interne :** Une énumération `MyEspNowMessageType` est utilisée en interne pour distinguer les deux types de formats de données.
- **Nouvelles Fonctions :** `sendPacket` et `setOnPacketReceivedCallback` ont été ajoutées pour gérer le nouveau système.

```cpp
// Fichier : MyEspNow.h (Version 4 - Actuelle)
#ifndef MY_ESP_NOW_H
#define MY_ESP_NOW_H

#include <esp_now.h>
#include <WiFi.h>
#include <functional>

// Énumération pour les commandes (utilisée par l'ancien système)
enum CommandType {
    CMD_CHANGE_PAGE,
    CMD_SENSOR_DATA,
    CMD_ACK
};

// Structure de données originale (legacy)
struct MyEspNowData {
    CommandType cmd;
    int id;
    float value1;
    float value2;
    char text[100];
};

// Énumération interne pour différencier les types de messages
enum MyEspNowMessageType : uint8_t {
    TYPE_LEGACY_DATA = 0x01,
    TYPE_GENERIC_PACKET = 0x02
};

// Callback pour l'ancien format
using EspNowDataReceivedCallback = std::function<void(const uint8_t* mac_addr, const MyEspNowData& data)>;
// Callback pour le nouveau format de paquet générique
using EspNowPacketReceivedCallback = std::function<void(const uint8_t* mac_addr, const uint8_t* data, uint8_t len)>;

class MyEspNow {
public:
    MyEspNow();
    bool begin();

    // Fonctions pour la structure de données originale
    void setOnDataReceivedCallback(EspNowDataReceivedCallback callback);
    bool sendData(const uint8_t* peer_addr, const MyEspNowData& data);
    bool sendWithAck(const uint8_t* peer_addr, MyEspNowData& data, int retries = 5, int ack_timeout_ms = 200);

    // Nouvelles fonctions pour l'envoi de données génériques
    void setOnPacketReceivedCallback(EspNowPacketReceivedCallback callback);
    bool sendPacket(const uint8_t* peer_addr, const uint8_t* data, size_t len);

    bool addPeer(const uint8_t* peer_addr);

    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);
    static void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);

private:
    static EspNowDataReceivedCallback onDataReceived;
    static EspNowPacketReceivedCallback onPacketReceived;

    static volatile bool appAckReceived;
    static int waitingForAckId;
};

#endif // MY_ESP_NOW_H
```
