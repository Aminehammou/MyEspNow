/**
 * @file MyEspNow.h
 * @brief Définit une classe wrapper pour simplifier l'utilisation du protocole ESP-NOW.
 * 
 * Cette bibliothèque encapsule l'initialisation, l'envoi, la réception de données,
 * et un mécanisme de découverte de pairs pour les appareils ESP32 utilisant ESP-NOW.
 */

#ifndef MY_ESP_NOW_H
#define MY_ESP_NOW_H

#include <esp_now.h>
#include <WiFi.h>
#include <functional>
#include <vector>

/**
 * @enum CommandType
 * @brief Définit les types de commandes qui peuvent être envoyées dans un paquet de données.
 */
enum CommandType {
    CMD_CHANGE_PAGE,        ///< Commande pour changer de page sur l'appareil distant.
    CMD_SENSOR_DATA,        ///< Commande indiquant que le paquet contient des données de capteur.
    CMD_ACK,                ///< Commande pour un accusé de réception applicatif.
    CMD_DISCOVERY_REQUEST,  ///< Commande pour une requête de découverte de pairs.
    CMD_DISCOVERY_RESPONSE, ///< Commande pour une réponse à une requête de découverte.
};

/**
 * @struct MyEspNowData
 * @brief Structure de données standard pour l'échange d'informations.
 * @warning La taille totale de cette structure ne doit pas dépasser 250 octets.
 */
struct MyEspNowData {
    CommandType cmd;    ///< Le type de commande du paquet.
    int id;             ///< Un identifiant unique pour le message, utilisé pour les ACK.
    float value1;       ///< Une première valeur flottante générique.
    float value2;       ///< Une seconde valeur flottante générique.
    char text[100];     ///< Un champ de texte pour des données flexibles.
};

/**
 * @struct DiscoveryPacket
 * @brief Structure de données spécifique pour les paquets de découverte.
 */
struct DiscoveryPacket {
    CommandType cmd;        ///< Doit être CMD_DISCOVERY_REQUEST ou CMD_DISCOVERY_RESPONSE.
    uint8_t mac_addr[6];    ///< L'adresse MAC de l'appareil qui envoie le paquet.
    char name[32];          ///< Le nom textuel de l'appareil.
};

/**
 * @enum MyEspNowMessageType
 * @brief Énumération interne pour différencier les types de paquets au niveau de la couche de transport.
 */
enum MyEspNowMessageType : uint8_t {
    TYPE_LEGACY_DATA = 0x01,      ///< Pour les paquets utilisant la structure MyEspNowData.
    TYPE_GENERIC_PACKET = 0x02,   ///< Pour les paquets de données brutes (génériques).
    TYPE_DISCOVERY_PACKET = 0x03, ///< Pour les paquets de découverte.
};

// --- Types de fonctions de rappel (Callbacks) ---

/**
 * @brief Callback pour la réception de données structurées (MyEspNowData).
 * @param mac_addr L'adresse MAC de l'expéditeur.
 * @param data La structure de données reçue.
 */
using EspNowDataReceivedCallback = std::function<void(const uint8_t* mac_addr, const MyEspNowData& data)>;

/**
 * @brief Callback pour la réception de paquets de données génériques (brutes).
 * @param mac_addr L'adresse MAC de l'expéditeur.
 * @param data Pointeur vers les données brutes reçues.
 * @param len La longueur des données reçues.
 */
using EspNowPacketReceivedCallback = std::function<void(const uint8_t* mac_addr, const uint8_t* data, uint8_t len)>;

/**
 * @brief Callback pour la découverte d'un nouveau pair.
 * @param mac_addr L'adresse MAC du pair qui a répondu à la découverte.
 * @param name Le nom du pair découvert.
 */
using PeerDiscoveryCallback = std::function<void(const uint8_t* mac_addr, const char* name)>;


/**
 * @class MyEspNow
 * @brief Classe principale qui gère les fonctionnalités ESP-NOW.
 */
class MyEspNow {
public:
    /**
     * @brief Constructeur de la classe MyEspNow.
     */
    MyEspNow();

    /**
     * @brief Initialise le WiFi en mode STA et le service ESP-NOW.
     * @return `true` si l'initialisation est réussie, `false` sinon.
     */
    bool begin();

    /**
     * @brief Définit le nom de cet appareil pour la découverte.
     * @param name Le nom de l'appareil (max 31 caractères).
     */
    void setDeviceName(const char* name);
    
    // --- Fonctions pour la structure de données originale ---

    /**
     * @brief Définit la fonction de rappel à appeler lors de la réception de données structurées (MyEspNowData).
     * @param callback La fonction à appeler.
     */
    void setOnDataReceivedCallback(EspNowDataReceivedCallback callback);

    /**
     * @brief Envoie des données structurées (MyEspNowData) à un pair.
     * @param peer_addr L'adresse MAC du destinataire.
     * @param data La structure de données à envoyer.
     * @return `true` si l'envoi a été mis en file d'attente avec succès, `false` sinon.
     */
    bool sendData(const uint8_t* peer_addr, const MyEspNowData& data);
    
    // --- Nouvelles fonctions pour l'envoi de données génériques ---

    /**
     * @brief Définit la fonction de rappel pour les paquets de données génériques.
     * @param callback La fonction à appeler.
     */
    void setOnPacketReceivedCallback(EspNowPacketReceivedCallback callback);

    /**
     * @brief Envoie un paquet de données brutes (génériques) à un pair.
     * @param peer_addr L'adresse MAC du destinataire.
     * @param data Pointeur vers les données à envoyer.
     * @param len La longueur des données à envoyer.
     * @return `true` si l'envoi a été mis en file d'attente avec succès, `false` sinon.
     */
    bool sendPacket(const uint8_t* peer_addr, const uint8_t* data, size_t len);

    // --- Fonctions pour la découverte des pairs ---

    /**
     * @brief Définit la fonction de rappel à appeler lorsqu'un pair est découvert.
     * @param callback La fonction à appeler.
     */
    void setOnPeerDiscoveredCallback(PeerDiscoveryCallback callback);

    /**
     * @brief Diffuse un paquet de requête de découverte sur le réseau.
     */
    void discoverPeers();

    /**
     * @brief Envoie des données de manière fiable avec un mécanisme d'accusé de réception applicatif.
     * @param peer_addr L'adresse MAC du destinataire.
     * @param data La structure de données à envoyer. Son champ `id` sera modifié.
     * @param retries Nombre de tentatives d'envoi.
     * @param ack_timeout_ms Délai d'attente pour l'ACK en millisecondes.
     * @return `true` si l'ACK a été reçu, `false` sinon.
     */
    bool sendWithAck(const uint8_t* peer_addr, MyEspNowData& data, int retries = 5, int ack_timeout_ms = 200);
    
    /**
     * @brief Ajoute un appareil à la liste des pairs ESP-NOW.
     * @param peer_addr L'adresse MAC du pair à ajouter.
     * @return `true` si le pair a été ajouté ou s'il existait déjà, `false` en cas d'erreur.
     */
    static bool addPeer(const uint8_t* peer_addr);
    
    // --- Callbacks statiques internes (utilisés par ESP-NOW) ---

    /**
     * @brief Callback statique appelé par ESP-NOW après une tentative d'envoi.
     * @param mac_addr L'adresse MAC du destinataire.
     * @param status Le statut de l'envoi (succès ou échec).
     */
    static void onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);

    /**
     * @brief Callback statique appelé par ESP-NOW lors de la réception de données.
     * @param mac_addr L'adresse MAC de l'expéditeur.
     * @param incomingData Les données reçues.
     * @param len La longueur des données.
     */
    static void onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len);

private:
    /// @brief Le nom de cet appareil, utilisé lors de la découverte.
    static char deviceName[32];
    /// @brief Pointeur statique vers la fonction de rappel de l'utilisateur pour les données structurées.
    static EspNowDataReceivedCallback onDataReceived;
    /// @brief Pointeur statique vers la fonction de rappel de l'utilisateur pour les paquets génériques.
    static EspNowPacketReceivedCallback onPacketReceived;
    /// @brief Pointeur statique vers la fonction de rappel de l'utilisateur pour la découverte de pairs.
    static PeerDiscoveryCallback onPeerDiscovered;

    // --- Variables pour le mécanisme d'ACK applicatif ---
    /// @brief Indicateur volatile pour signaler la réception d'un ACK.
    static volatile bool appAckReceived;
    /// @brief Stocke l'ID du message pour lequel un ACK est attendu.
    static int waitingForAckId;
};

#endif // MY_ESP_NOW_H
