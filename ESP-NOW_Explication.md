Ce guide explique comment utiliser la bibliothèque `MyEspNow` qui supporte deux modes de communication :

1.  **Mode Données Structurées (Legacy) :** Idéal pour des échanges simples et rapides avec une structure de données prédéfinie (`MyEspNowData`). Il intègre un mécanisme d'accusé de réception (`sendWithAck`) pour une communication fiable.
2.  **Mode Paquet Générique :** Offre une flexibilité maximale en vous permettant d'envoyer n'importe quelle structure de données personnalisée (`sendPacket`), jusqu'à 249 octets.

## Scénario : Un Capteur et un Afficheur

Nous allons illustrer les deux modes de communication avec un système composé de deux ESP32 :

*   **L'Émetteur :** Envoie des données de capteurs simulées en utilisant à la fois l'ancienne et la nouvelle méthode.
*   **Le Récepteur :** Reçoit et traite les deux types de messages, en affichant les informations sur un écran OLED.

---

### Étape 1 : Prérequis - Connaître les Adresses MAC

Chaque module ESP32 a une adresse MAC unique. Pour communiquer, ils doivent se connaître. Utilisez ce sketch pour obtenir l'adresse MAC de chaque ESP32 :

```cpp
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.print("Adresse MAC de cet ESP32 : ");
  Serial.println(WiFi.macAddress());
}

void loop() {}
```

---

### Étape 2 : Le Code du Récepteur (Afficheur OLED)

Ce code, disponible dans `examples/Receiver/Receiver.ino`, montre comment gérer les deux types de messages.

**Objectif :** Recevoir et traiter les données des deux formats.

```cpp
// ... (début du code, déclarations, etc.)

// Structure personnalisée pour le nouveau format
struct SensorDataPacket {
    float temperature;
    float humidity;
    int pressure;
    char description[50];
};

// Callback pour l'ancien format
void onDataReceived(const uint8_t* mac_addr, const MyEspNowData& data) {
    // ... (logique pour traiter MyEspNowData)
}

// Callback pour le nouveau format
void onPacketReceived(const uint8_t* mac_addr, const uint8_t* data, uint8_t len) {
    if (len == sizeof(SensorDataPacket)) {
        SensorDataPacket packet;
        memcpy(&packet, data, len);
        // ... (logique pour traiter votre paquet personnalisé)
    }
}

void setup() {
    // ... (initialisation Serial, OLED, ESP-NOW)

    // Enregistrer les deux fonctions de callback
    espnow.setOnDataReceivedCallback(onDataReceived);
    espnow.setOnPacketReceivedCallback(onPacketReceived);
}

// ... (le reste du code)
```

---

### Étape 3 : Le Code de l'Émetteur (Capteur)

Ce code, de `examples/Transmitter/Transmitter.ino`, envoie les deux types de messages.

**Objectif :** Envoyer des données en utilisant `sendWithAck` et `sendPacket`.

```cpp
// ... (début du code, déclarations, etc.)

// Structure personnalisée pour le nouveau format
struct SensorDataPacket {
    float temperature;
    float humidity;
    int pressure;
    char description[50];
};

void loop() {
    if (/* bouton pressé */) {
        // --- Envoi avec l'ancienne méthode ---
        MyEspNowData legacyData;
        // ... (remplir legacyData)
        espnow.sendWithAck(recepteurAddress, legacyData);

        // --- Envoi avec la nouvelle méthode ---
        SensorDataPacket packetData;
        // ... (remplir packetData)
        espnow.sendPacket(recepteurAddress, (const uint8_t*)&packetData, sizeof(packetData));
    }
}
```

### Comment Utiliser

1.  **Téléversement :**
    *   Ouvrez `Receiver.ino`, remplacez l'adresse MAC de l'émetteur, et téléversez.
    *   Ouvrez `Transmitter.ino`, remplacez l'adresse MAC du récepteur, et téléversez.
2.  **Exécution :**
    *   Alimentez les deux ESP32 et ouvrez leurs moniteurs série.
    *   Appuyez sur le bouton de l'émetteur.

### Résultat Attendu

*   **Émetteur :** Le moniteur série affichera la confirmation d'envoi pour les deux formats.
*   **Récepteur :** Le moniteur série et l'écran OLED afficheront les données reçues des deux formats, prouvant que les deux systèmes de callbacks fonctionnent en parallèle.
