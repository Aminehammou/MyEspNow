# Bibliothèque MyEspNow

Une bibliothèque pour simplifier la communication ESP-NOW sur ESP32, avec une gestion fiable des messages grâce à un système d'accusé de réception (ACK) applicatif.

## Fonctionnalités

- Initialisation simplifiée du mode Wi-Fi et d'ESP-NOW.
- Ajout facile de pairs (appareils distants).
- Envoi de données structurées (`MyEspNowData`).
- Réception de données via une fonction de rappel (callback).
- **Envoi fiable** : un mécanisme `sendWithAck` qui attend un accusé de réception applicatif et effectue des tentatives en cas d'échec.
- **Gestion des messages** : les messages sont identifiés par un `id` unique pour le suivi des ACKs.

## Structure des données

La communication est basée sur la structure `MyEspNowData` :

```cpp
enum CommandType {
    CMD_CHANGE_PAGE,
    CMD_SENSOR_DATA,
    CMD_ACK // Accusé de réception
};

struct MyEspNowData {
    CommandType cmd;
    int id;
    float value1;
    float value2;
    char text[100];
};
```

- `cmd` : Le type de commande (par exemple, changer de page, envoyer des données de capteur).
- `id` : Un identifiant unique pour chaque message, utilisé pour le mécanisme d'ACK.
- `value1`, `value2` : Valeurs numériques (par exemple, température, humidité).
- `text` : Une chaîne de caractères pour des données textuelles.

## Installation

1. Téléchargez les fichiers `MyEspNow.h` et `MyEspNow.cpp`.
2. Placez-les dans le même répertoire que votre fichier `.ino` principal.
3. Incluez la bibliothèque dans votre projet avec `#include "MyEspNow.h"`.

## Utilisation

### 1. Initialisation

```cpp
#include "MyEspNow.h"

MyEspNow espNow;

void setup() {
    Serial.begin(115200);
    if (espNow.begin()) {
        Serial.println("ESP-NOW initialisé avec succès.");
    } else {
        Serial.println("Erreur d'initialisation d'ESP-NOW.");
    }
}
```

### 2. Ajout d'un pair

Chaque appareil avec lequel vous souhaitez communiquer doit être ajouté comme un pair.

```cpp
// Adresse MAC du récepteur
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

void setup() {
    // ... initialisation ...
    if (!espNow.addPeer(broadcastAddress)) {
        Serial.println("Échec de l'ajout du pair.");
    }
}
```

### 3. Réception de données

Définissez une fonction de rappel qui sera exécutée à chaque fois qu'un message est reçu.

```cpp
void onDataReceived(const uint8_t* mac_addr, const MyEspNowData& data) {
    Serial.printf("Message reçu de %02X:%02X:%02X:%02X:%02X:%02X\n",
                  mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.printf("Commande: %d, ID: %d, Valeur1: %.2f, Valeur2: %.2f, Texte: %s\n",
                  data.cmd, data.id, data.value1, data.value2, data.text);

    // Si le message n'est pas un ACK, renvoyer un ACK
    if (data.cmd != CMD_ACK) {
        MyEspNowData ackData;
        ackData.cmd = CMD_ACK;
        ackData.id = data.id; // Renvoyer le même ID
        espNow.sendData(mac_addr, ackData); // Envoyer l'ACK
    }
}

void setup() {
    // ... initialisation ...
    espNow.setOnDataReceivedCallback(onDataReceived);
}
```

### 4. Envoi de données

#### Envoi simple (sans garantie de réception)

```cpp
MyEspNowData data;
data.cmd = CMD_SENSOR_DATA;
data.value1 = 25.5;
strcpy(data.text, "Température");

if (espNow.sendData(broadcastAddress, data)) {
    Serial.println("Données envoyées avec succès.");
} else {
    Serial.println("Échec de l'envoi des données.");
}
```

#### Envoi fiable avec accusé de réception (`sendWithAck`)

Cette fonction envoie un message et attend un `CMD_ACK` en retour. Elle réessaie plusieurs fois si l'ACK n'est pas reçu dans le délai imparti.

```cpp
MyEspNowData data;
data.cmd = CMD_CHANGE_PAGE;
data.id = -1; // L'ID sera défini par la fonction
strcpy(data.text, "HomePage");

// Tenter d'envoyer avec 5 tentatives et un timeout de 200ms pour l'ACK
if (espNow.sendWithAck(broadcastAddress, data, 5, 200)) {
    Serial.println("Message envoyé et ACK reçu !");
} else {
    Serial.println("Échec de l'envoi après plusieurs tentatives.");
}
```

## API de la bibliothèque

### `bool begin()`

Initialise le Wi-Fi en mode `WIFI_STA` et ESP-NOW. Affiche l'adresse MAC de l'appareil sur le port série.
**Retourne** : `true` si l'initialisation est réussie, `false` sinon.

### `void setOnDataReceivedCallback(EspNowDataReceivedCallback callback)`

Définit la fonction de rappel à exécuter lors de la réception de données.

- `callback` : Une fonction avec la signature `void(const uint8_t* mac_addr, const MyEspNowData& data)`.

### `bool addPeer(const uint8_t* peer_addr)`

Ajoute un appareil à la liste des pairs.

- `peer_addr` : L'adresse MAC du pair.
**Retourne** : `true` si le pair est ajouté avec succès, `false` sinon.

### `bool sendData(const uint8_t* peer_addr, const MyEspNowData& data)`

Envoie des données à un pair sans attendre d'accusé de réception applicatif.

- `peer_addr` : L'adresse MAC du destinataire.
- `data` : La structure `MyEspNowData` à envoyer.
**Retourne** : `true` si l'envoi a été mis en file d'attente avec succès (ne garantit pas la réception).

### `bool sendWithAck(const uint8_t* peer_addr, MyEspNowData& data, int retries = 5, int ack_timeout_ms = 200)`

Envoie des données de manière fiable. Un `id` unique est assigné au message. La fonction attend un message `CMD_ACK` avec le même `id`.

- `peer_addr` : L'adresse MAC du destinataire.
- `data` : La structure `MyEspNowData` à envoyer. L'`id` est modifié par la fonction.
- `retries` : Le nombre de tentatives d'envoi.
- `ack_timeout_ms` : Le temps d'attente pour l'ACK en millisecondes.
**Retourne** : `true` si l'ACK a été reçu, `false` après l'échec de toutes les tentatives.
