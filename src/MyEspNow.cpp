// Définition des membres statiques
EspNowDataReceivedCallback MyEspNow::onDataReceived = nullptr;
EspNowPacketReceivedCallback MyEspNow::onPacketReceived = nullptr;
volatile bool MyEspNow::appAckReceived = false;
int MyEspNow::waitingForAckId = -1;

MyEspNow::MyEspNow() {}

bool MyEspNow::begin() {
    WiFi.mode(WIFI_STA);
    Serial.print("Adresse MAC de cet ESP32 : ");
    Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("Erreur lors de l'initialisation d'ESP-NOW");
        return false;
    }

    esp_now_register_send_cb(MyEspNow::onDataSent);
    esp_now_register_recv_cb(MyEspNow::onDataRecv);

    return true;
}

void MyEspNow::setOnDataReceivedCallback(EspNowDataReceivedCallback callback) {
    onDataReceived = callback;
}

void MyEspNow::setOnPacketReceivedCallback(EspNowPacketReceivedCallback callback) {
    onPacketReceived = callback;
}

bool MyEspNow::addPeer(const uint8_t* peer_addr) {
    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, peer_addr, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Échec de l'ajout du pair");
        return false;
    }
    return true;
}

bool MyEspNow::sendData(const uint8_t* peer_addr, const MyEspNowData& data) {
    uint8_t buffer[sizeof(MyEspNowData) + 1];
    buffer[0] = TYPE_LEGACY_DATA;
    memcpy(buffer + 1, &data, sizeof(data);
    esp_err_t result = esp_now_send(peer_addr, buffer, sizeof(buffer));
    return result == ESP_OK;
}

bool MyEspNow::sendPacket(const uint8_t* peer_addr, const uint8_t* data, size_t len) {
    if (len > 249) { // 250 - 1 octet pour le type
        Serial.println("Erreur: La taille des données dépasse la limite de 249 octets.");
        return false;
    }
    uint8_t buffer[len + 1];
    buffer[0] = TYPE_GENERIC_PACKET;
    memcpy(buffer + 1, data, len);
    esp_err_t result = esp_now_send(peer_addr, buffer, sizeof(buffer));
    return result == ESP_OK;
}


bool MyEspNow::sendWithAck(const uint8_t* peer_addr, MyEspNowData& data, int retries, int ack_timeout_ms) {
    static int messageId = 0;
    data.id = messageId++;
    waitingForAckId = data.id;

    for (int i = 0; i < retries; i++) {
        appAckReceived = false;
        if (sendData(peer_addr, data)) {
            Serial.printf("Tentative %d/%d: Envoi du message ID %d...\n", i + 1, retries, data.id);
        } else {
            Serial.printf("Tentative %d/%d: Échec de l'envoi du message ID %d.\n", i + 1, retries, data.id);
            delay(100);
            continue;
        }

        unsigned long ack_wait_start = millis();
        while (!appAckReceived && (millis() - ack_wait_start < ack_timeout_ms)) {
            delay(1);
        }

        if (appAckReceived) {
            Serial.printf("ACK applicatif reçu pour l'ID %d.\n", data.id);
            waitingForAckId = -1;
            return true;
        }
    }
    Serial.printf("Échec final de l'envoi du message ID %d après %d tentatives.\n", data.id, retries);
    waitingForAckId = -1;
    return false;
}

void MyEspNow::onDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "ACK MAC: Envoi réussi" : "ACK MAC: Échec de l'envoi");
}

void MyEspNow::onDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
    if (len == 0) return;

    uint8_t type = incomingData[0];
    const uint8_t* data = incomingData + 1;
    int dataLen = len - 1;

    if (type == TYPE_LEGACY_DATA) {
        if (dataLen <= sizeof(MyEspNowData) && onDataReceived != nullptr) {
            MyEspNowData receivedData;
            memcpy(&receivedData, data, dataLen);

            if (receivedData.cmd == CMD_ACK && receivedData.id == waitingForAckId) {
                appAckReceived = true;
            }
            onDataReceived(mac_addr, receivedData);
        }
    } else if (type == TYPE_GENERIC_PACKET) {
        if (onPacketReceived != nullptr) {
            onPacketReceived(mac_addr, data, dataLen);
        }
    }
}