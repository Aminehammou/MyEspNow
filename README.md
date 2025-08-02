Une bibliothèque ESP32 pour une communication ESP-NOW simple et flexible, supportant à la fois des structures de données fixes et des paquets de données génériques.

## Fonctionnalités

- **Deux Modes de Communication :**
  1. **Mode Données Structurées :** Utilisez la structure `MyEspNowData` pour des messages simples et un envoi fiable avec `sendWithAck`.
  2. **Mode Paquet Générique :** Envoyez n'importe quelle structure de données personnalisée (jusqu'à 249 octets) avec `sendPacket` pour une flexibilité maximale.
- **Callbacks Doubles :** Des fonctions de rappel distinctes pour chaque mode de communication (`onDataReceived` et `onPacketReceived`).
- **Fiabilité :** Le mode structuré inclut un mécanisme d'accusé de réception (ACK) avec tentatives.
- **Simplicité :** Facile à initialiser et à utiliser.

## Installation

1. Placez les fichiers `MyEspNow.h` et `MyEspNow.cpp` dans le même répertoire que votre fichier `.ino`.
2. Incluez la bibliothèque : `#include "MyEspNow.h"`.

## Utilisation Rapide

### 1. Initialisation et Ajout d'un Pair

```cpp
#include "MyEspNow.h"

MyEspNow espNow;
// Adresse MAC du récepteur
uint8_t peerAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

void setup() {
    Serial.begin(115200);
    espNow.begin();
    espNow.addPeer(peerAddress);
}
```

### 2. Envoyer et Recevoir

La bibliothèque gère deux types de formats de données en parallèle.

#### Mode 1 : Données Structurées (Fiable)

Idéal pour des commandes simples où la fiabilité est clé.

**Émetteur :**

```cpp
MyEspNowData data;
data.cmd = CMD_SENSOR_DATA;
data.value1 = 25.5;

if (espNow.sendWithAck(peerAddress, data)) {
    Serial.println("Message structuré envoyé avec succès !");
}
```

**Récepteur :**

```cpp
void onDataReceived(const uint8_t* mac, const MyEspNowData& data) {
    Serial.printf("Valeur reçue: %.2f\n", data.value1);
    // Renvoyer un ACK pour la fonction sendWithAck
    MyEspNowData ackData; 
    ackData.cmd = CMD_ACK; 
    ackData.id = data.id; 
    espNow.sendData(mac, ackData);
}

espNow.setOnDataReceivedCallback(onDataReceived);
```

#### Mode 2 : Paquet de Données Générique

Parfait pour envoyer des structures de données personnalisées et plus volumineuses.

**Émetteur :**

```cpp
struct CustomPacket {
    float temp;
    int id;
    char name[20];
};

CustomPacket myPacket = {36.6, 123, "Station1"};
espNow.sendPacket(peerAddress, (const uint8_t*)&myPacket, sizeof(myPacket));
```

**Récepteur :**

```cpp
void onPacketReceived(const uint8_t* mac, const uint8_t* data, uint8_t len) {
    if (len == sizeof(CustomPacket)) {
        CustomPacket receivedPacket;
        memcpy(&receivedPacket, data, len);
        Serial.printf("Paquet reçu de %s\n", receivedPacket.name);
    }
}

espNow.setOnPacketReceivedCallback(onPacketReceived);
```

## API de la Bibliothèque

- `bool begin()`: Initialise ESP-NOW.
- `bool addPeer(const uint8_t* peer_addr)`: Ajoute un pair.

**Mode Données Structurées :**

- `void setOnDataReceivedCallback(callback)`: Définit le callback pour les messages structurés.
- `bool sendData(peer, data)`: Envoie une structure `MyEspNowData`.
- `bool sendWithAck(peer, data, ...)`: Envoie `MyEspNowData` et attend un ACK.

**Mode Paquet Générique :**

- `void setOnPacketReceivedCallback(callback)`: Définit le callback pour les paquets génériques.
- `bool sendPacket(peer, data, len)`: Envoie un tableau d'octets de longueur `len`.
