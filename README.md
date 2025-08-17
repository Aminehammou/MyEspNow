# MyEspNow - Bibliothèque ESP-NOW Simplifiée pour ESP32

`MyEspNow` est une bibliothèque C++ conçue pour simplifier l'utilisation du protocole de communication ESP-NOW sur les microcontrôleurs ESP32. Elle fournit une interface de haut niveau qui abstrait la complexité de l'API ESP-NOW native, rendant la communication sans fil entre les appareils ESP32 plus facile et plus intuitive.

Cette bibliothèque est idéale pour des projets nécessitant une communication à faible latence et sans connexion Wi-Fi formelle, comme les télécommandes, les réseaux de capteurs, ou la communication entre robots.

## Fonctionnalités

*   **Initialisation Facile** : Démarrez avec ESP-NOW en une seule ligne de code.
*   **Deux Modes de Communication** :
    1.  **Structuré et Fiable** : Envoyez des données via une structure prédéfinie (`MyEspNowData`) avec un mécanisme d'accusé de réception (ACK) et de nouvelles tentatives automatiques pour garantir la livraison.
    2.  **Paquet Générique** : Envoyez n'importe quelle structure de données personnalisée pour une flexibilité maximale.
*   **Découverte Automatique de Pairs** : Diffusez un message pour découvrir d'autres appareils sur le réseau et vous y connecter automatiquement.
*   **Callbacks Modernes** : Utilisez des `std::function` pour une gestion simple et flexible des événements de réception de données.
*   **Gestion des Pairs** : Ajoutez et gérez facilement les appareils avec lesquels vous souhaitez communiquer.

## Installation

1.  **Téléchargez la Bibliothèque** : Clonez ce dépôt ou téléchargez les fichiers `MyEspNow.h` et `MyEspNow.cpp`.
2.  **Intégrez à votre Projet** :
    *   **PlatformIO** : Placez le dossier `MyEspNow` dans le répertoire `lib` de votre projet PlatformIO.
    *   **Arduino IDE** : Créez un dossier nommé `MyEspNow` dans le répertoire `libraries` de votre carnet de croquis Arduino, et placez-y les fichiers `MyEspNow.h` et `MyEspNow.cpp`.
3.  **Incluez la Bibliothèque** : Ajoutez la ligne suivante en haut de votre fichier de code principal :
    ```cpp
    #include "MyEspNow.h"
    ```

## Guide de Démarrage Rapide

Voici un exemple simple pour envoyer un message d'un émetteur à un récepteur.

**Code du Récepteur :**

```cpp
#include <Arduino.h>
#include "MyEspNow.h"

MyEspNow espNow;

// Cette fonction est appelée lorsque des données structurées sont reçues.
void onDataReceived(const uint8_t* mac_addr, const MyEspNowData& data) {
    Serial.print("Message reçu de : ");
    // Affiche l'adresse MAC de l'expéditeur
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.println(macStr);

    Serial.printf("Commande: %d, Valeur1: %.2f, Valeur2: %.2f, Texte: %s
",
                  data.cmd, data.value1, data.value2, data.text);
}

void setup() {
    Serial.begin(115200);
    Serial.println("Initialisation du Récepteur ESP-NOW");

    // Initialise la bibliothèque
    espNow.begin();

    // Attache la fonction de callback
    espNow.setOnDataReceivedCallback(onDataReceived);
}

void loop() {
    // Le programme principal peut faire autre chose
    delay(1000);
}
```

**Code de l'Émetteur :**

```cpp
#include <Arduino.h>
#include "MyEspNow.h"

MyEspNow espNow;

// Remplacez par l'adresse MAC de votre récepteur
uint8_t receiverAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}; // Mettez ici l'adresse MAC du récepteur

void setup() {
    Serial.begin(115200);
    Serial.println("Initialisation de l'Émetteur ESP-NOW");

    // Initialise la bibliothèque
    espNow.begin();

    // Ajoute le récepteur comme pair
    espNow.addPeer(receiverAddress);
}

void loop() {
    // Prépare les données à envoyer
    MyEspNowData data;
    data.cmd = CMD_SENSOR_DATA;
    data.value1 = random(0, 100); // Valeur de capteur aléatoire
    data.value2 = 3.14;
    strcpy(data.text, "Bonjour ESP-NOW !");

    // Envoie les données
    if (espNow.sendData(receiverAddress, data)) {
        Serial.println("Message envoyé avec succès !");
    } else {
        Serial.println("Échec de l'envoi du message.");
    }

    delay(5000); // Envoie un message toutes les 5 secondes
}
```

## Utilisation Avancée

### 1. Envoi Fiable avec `sendWithAck`

Pour garantir qu'un message a bien été reçu, vous pouvez utiliser `sendWithAck`. Cette fonction attend un accusé de réception (ACK) de la part du récepteur. La bibliothèque gère automatiquement l'envoi de l'ACK lorsque le récepteur reçoit un message.

**Émetteur :**
```cpp
MyEspNowData data;
data.cmd = CMD_CHANGE_PAGE;
data.value1 = 1; // Changer pour la page 1

// Tente d'envoyer le message jusqu'à 5 fois, avec un timeout de 200ms pour l'ACK
if (espNow.sendWithAck(receiverAddress, data, 5, 200)) {
    Serial.println("Message envoyé et ACK reçu !");
} else {
    Serial.println("Échec de l'envoi après plusieurs tentatives.");
}
```

**Récepteur :**
Le code du récepteur reste le même que dans l'exemple de base. La bibliothèque détecte que le message attend un ACK et s'en occupe pour vous.

### 2. Envoi de Données Personnalisées avec `sendPacket`

Si la structure `MyEspNowData` ne convient pas à vos besoins, vous pouvez envoyer n'importe quelle structure de données.

**Émetteur :**
```cpp
struct CustomPacket {
    int id;
    float temperature;
    char sensorName[16];
};

CustomPacket myPacket = {123, 25.7, "Salon"};

espNow.sendPacket(receiverAddress, (const uint8_t*)&myPacket, sizeof(myPacket));
```

**Récepteur :**
Vous devez utiliser un callback différent pour les paquets génériques.

```cpp
// Callback pour les paquets de données génériques
void onPacketReceived(const uint8_t* mac_addr, const uint8_t* data, uint8_t len) {
    if (len == sizeof(CustomPacket)) {
        CustomPacket receivedPacket;
        memcpy(&receivedPacket, data, len);
        Serial.printf("Paquet personnalisé reçu : ID=%d, Temp=%.1f, Nom=%s
",
                      receivedPacket.id, receivedPacket.temperature, receivedPacket.sensorName);
    }
}

// Dans setup()
espNow.setOnPacketReceivedCallback(onPacketReceived);
```

### 3. Découverte de Pairs

Si vous ne connaissez pas les adresses MAC des autres appareils, vous pouvez utiliser la fonction de découverte.

**Appareil 1 (qui cherche) :**
```cpp
// Callback appelé lorsqu'un pair répond
void onPeerDiscovered(const uint8_t* mac_addr, const char* name) {
    Serial.print("Pair découvert ! Nom: ");
    Serial.print(name);
    Serial.print(", MAC: ");
    // Affiche l'adresse MAC
    // ...
    
    // Ajoute le pair pour pouvoir communiquer avec lui
    espNow.addPeer(mac_addr);
}

void setup() {
    // ...
    espNow.begin();
    espNow.setDeviceName("Appareil Chercheur");
    espNow.setOnPeerDiscoveredCallback(onPeerDiscovered);
    
    // Lance la découverte
    espNow.discoverPeers();
}
```

**Appareil 2 (qui répond) :**
```cpp
void setup() {
    // ...
    espNow.begin();
    espNow.setDeviceName("Appareil Répondeur");
    // Le répondeur n'a rien de spécial à faire, la bibliothèque gère la réponse automatiquement.
}
```

## Référence de l'API

### Fonctions Principales
*   `bool begin()`: Initialise le WiFi et ESP-NOW. Doit être appelée dans `setup()`.
*   `void setDeviceName(const char* name)`: Définit un nom pour cet appareil, utilisé lors de la découverte.
*   `bool addPeer(const uint8_t* peer_addr)`: Ajoute un appareil à la liste des pairs.

### Communication Structurée (`MyEspNowData`)
*   `void setOnDataReceivedCallback(EspNowDataReceivedCallback callback)`: Définit la fonction à appeler lors de la réception d'une structure `MyEspNowData`.
*   `bool sendData(const uint8_t* peer_addr, const MyEspNowData& data)`: Envoie une structure de données.
*   `bool sendWithAck(const uint8_t* peer_addr, MyEspNowData& data, int retries = 5, int ack_timeout_ms = 200)`: Envoie une structure de données et attend un ACK.

### Communication Générique
*   `void setOnPacketReceivedCallback(EspNowPacketReceivedCallback callback)`: Définit la fonction à appeler lors de la réception d'un paquet de données générique.
*   `bool sendPacket(const uint8_t* peer_addr, const uint8_t* data, size_t len)`: Envoie un tableau d'octets bruts.

### Découverte
*   `void setOnPeerDiscoveredCallback(PeerDiscoveryCallback callback)`: Définit la fonction à appeler lorsqu'un pair est découvert.
*   `void discoverPeers()`: Diffuse une requête de découverte sur le réseau.

## Contribuer

Les contributions sont les bienvenues ! Si vous souhaitez améliorer cette bibliothèque, n'hésitez pas à forker le dépôt et à soumettre une Pull Request.

## Licence

Cette bibliothèque est distribuée sous la licence MIT. Voir le fichier `LICENSE` pour plus de détails.