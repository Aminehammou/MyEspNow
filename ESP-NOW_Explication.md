# Exemple d'Utilisation de la Bibliothèque MyEspNow

Ce guide explique comment mettre en place une communication bidirectionnelle fiable entre deux modules ESP32 en utilisant la bibliothèque `MyEspNow`.

## Scénario : Un Capteur et un Afficheur

Nous allons créer un système avec deux modules ESP32 :

1. **L'Émetteur (Le Capteur) :** Un ESP32 qui simule la lecture de capteurs (par exemple, température et humidité). Lorsqu'on appuie sur un bouton, il envoie ces données de manière fiable à l'afficheur.
2. **Le Récepteur (L'Afficheur) :** Un autre ESP32, connecté à un écran OLED. Il attend de recevoir les données du capteur. Lorsqu'il les reçoit, il les affiche sur l'écran et renvoie un accusé de réception (ACK) à l'émetteur pour confirmer que les données ont bien été reçues.

Cet exemple met en évidence la communication bidirectionnelle et l'utilisation de la fonction d'envoi fiable `sendWithAck`.

---

### Étape 1 : Prérequis - Connaître les Adresses MAC

Chaque module ESP32 a une adresse MAC unique. Pour que deux modules puissent communiquer via ESP-NOW, ils doivent se connaître mutuellement.

Voici un petit sketch Arduino à téléverser sur **chacun** de vos ESP32 pour connaître leur adresse MAC. Notez-les bien, nous en aurons besoin.

**Sketch pour obtenir l'adresse MAC :**

```cpp
#include <WiFi.h>

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  Serial.print("Adresse MAC de cet ESP32 : ");
  Serial.println(WiFi.macAddress());
}

void loop() {
}
```

---

### Étape 2 : Le Code du Récepteur (Afficheur OLED)

Ce code est pour l'ESP32 qui est connecté à l'écran OLED. Il est fourni dans le fichier `ESP-NOW_Recepteur.ino`.

**Objectif :** Recevoir les données et les afficher.

```cpp
// =================================================================
// ============== CODE POUR LE RÉCEPTEUR (AFFICHEUR) ===============
// =================================================================
#include <Arduino.h>
#include "MyEspNow.h"
#include "OledManager.h" // On inclut notre bibliothèque d'affichage

// --- Configuration ---
// Remplacez par l'adresse MAC de votre ÉMETTEUR
uint8_t emetteurAddress[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};

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
```

---

### Étape 3 : Le Code de l'Émetteur (Capteur)

Ce code est pour l'ESP32 avec un simple bouton-poussoir. Il est fourni dans le fichier `ESP-NOW_Emetteur.ino`.

**Objectif :** Envoyer les données de manière fiable lorsqu'on appuie sur le bouton.

```cpp
// =================================================================
// ================= CODE POUR L'ÉMETTEUR (CAPTEUR) =================
// =================================================================
#include <Arduino.h>
#include "MyEspNow.h"

// --- Configuration ---
// Remplacez par l'adresse MAC de votre RÉCEPTEUR
uint8_t recepteurAddress[] = {0xFF, 0xEE, 0xDD, 0xCC, 0xBB, 0xAA};

#define BUTTON_PIN 23 // Broche à laquelle le bouton est connecté (l'autre patte à GND)

// --- Objets globaux ---
MyEspNow espnow;

// --- Variables pour l'anti-rebond du bouton ---
unsigned long lastButtonPressTime = 0;
const long debounceDelay = 500; // 500 ms d'anti-rebond

void setup() {
    Serial.begin(115200);
    
    // Configurer le bouton avec une résistance de pull-up interne
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    // Initialiser ESP-NOW
    if (!espnow.begin()) {
        Serial.println("Erreur ESP-NOW");
        while(true);
    }

    // Ajouter le récepteur comme pair
    espnow.addPeer(recepteurAddress);
}

void loop() {
    // Vérifier si le bouton est pressé (LOW car on utilise INPUT_PULLUP)
    if (digitalRead(BUTTON_PIN) == LOW) {
        // Logique d'anti-rebond
        if (millis() - lastButtonPressTime > debounceDelay) {
            Serial.println("Bouton pressé ! Envoi des données...");
            
            // Créer la structure de données à envoyer
            MyEspNowData dataToSend;
            dataToSend.cmd = CMD_SENSOR_DATA;
            dataToSend.value1 = random(20, 30); // Simule une température
            dataToSend.value2 = random(40, 60); // Simule une humidité
            strcpy(dataToSend.text, "Test ESP-NOW");

            // Envoyer les données avec la méthode fiable
            if (espnow.sendWithAck(recepteurAddress, dataToSend)) {
                Serial.println("Envoi réussi avec ACK !");
            } else {
                Serial.println("Échec de l'envoi après plusieurs tentatives.");
            }

            // Mettre à jour le temps du dernier appui
            lastButtonPressTime = millis();
        }
    }
}
```

### Comment Utiliser

1. **Téléversement :**
    * Ouvrez le fichier `ESP-NOW_Recepteur.ino`. Remplacez l'adresse MAC `emetteurAddress` par celle de votre émetteur, puis téléversez le code sur l'ESP32 récepteur.
    * Ouvrez le fichier `ESP-NOW_Emetteur.ino`. Remplacez l'adresse MAC `recepteurAddress` par celle de votre récepteur, puis téléversez le code sur l'ESP32 émetteur.

2. **Exécution :**
    * Alimentez les deux ESP32.
    * Ouvrez le moniteur série pour les deux appareils.
    * Le récepteur affichera "En attente..." sur son écran OLED.
    * Appuyez sur le bouton de l'émetteur.

3. **Résultat Attendu :**
    * **Sur le moniteur de l'émetteur :** Vous verrez des messages indiquant les tentatives d'envoi, puis "Envoi réussi avec ACK !".
    * **Sur le moniteur du récepteur :** Vous verrez les détails du message reçu.
    * **Sur l'écran OLED :** L'écran s'actualisera pour afficher les nouvelles valeurs de température et d'humidité envoyées par l'émetteur.
