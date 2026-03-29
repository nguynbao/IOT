#include "BluetoothManager.h"
#include <Arduino.h>

// ==================== SERVER CALLBACKS ====================
class ServerCallbacks : public BLEServerCallbacks {
public:
    ServerCallbacks(BluetoothManager* btManager) : pBtManager(btManager) {}

    void onConnect(BLEServer* pServer) override {
        pBtManager->connected = true;
        Serial.println(" BLE Client connected");
        BLEDevice::startAdvertising();  // Restart advertising after connection
    }

    void onDisconnect(BLEServer* pServer) override {
        pBtManager->connected = false;
        Serial.println(" BLE Client disconnected");
        BLEDevice::startAdvertising();  // Restart advertising
    }

private:
    BluetoothManager* pBtManager;
};

// ==================== CHARACTERISTIC CALLBACKS ====================
class CharacteristicCallbacks : public BLECharacteristicCallbacks {
public:
    CharacteristicCallbacks(BluetoothManager* btManager) : pBtManager(btManager) {}

    void onWrite(BLECharacteristic* pCharacteristic) override {
        std::string value = pCharacteristic->getValue();

        if (value.length() > 0) {
            Serial.print(" BLE RX: ");
            for (uint8_t i = 0; i < value.length(); i++) {
                Serial.printf("%02X ", (uint8_t)value[i]);
            }
            Serial.println();

            // Invoke callback if registered
            if (pBtManager->dataReceivedCallback) {
                pBtManager->dataReceivedCallback((const uint8_t*)value.c_str(), value.length());
            }
        }
    }

private:
    BluetoothManager* pBtManager;
};

// ==================== BLUETOOTH MANAGER IMPLEMENTATION ====================
BluetoothManager::BluetoothManager()
    : pServer(nullptr),
      pService(nullptr),
      pCharTx(nullptr),
      pCharRx(nullptr),
      pCharStatus(nullptr),
      initialized(false),
      connected(false),
      dataReceivedCallback(nullptr) {}

BluetoothManager::~BluetoothManager() {
    // BLE cleanup handled by BLEDevice
}

bool BluetoothManager::begin(const char* deviceName) {
    if (initialized) {
        Serial.println(" BLE already initialized");
        return true;
    }

    try {
        // Initialize BLE Device
        BLEDevice::init(deviceName);
        Serial.printf(" BLE initialized: %s\n", deviceName);

        // Create BLE Server
        pServer = BLEDevice::createServer();
        if (!pServer) {
            Serial.println(" Failed to create BLE Server");
            return false;
        }
        pServer->setCallbacks(new ServerCallbacks(this));

        // Create BLE Service
        pService = pServer->createService(SERVICE_UUID);
        if (!pService) {
            Serial.println(" Failed to create BLE Service");
            return false;
        }

        // Create TX Characteristic (notify to client)
        pCharTx = pService->createCharacteristic(
            CHAR_TX_UUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
        );
        pCharTx->addDescriptor(new BLE2902());
        pCharTx->setValue("Ready");

        // Create RX Characteristic (write from client)
        pCharRx = pService->createCharacteristic(
            CHAR_RX_UUID,
            BLECharacteristic::PROPERTY_WRITE
        );
        pCharRx->setCallbacks(new CharacteristicCallbacks(this));

        // Create Status Characteristic (read/notify device status)
        pCharStatus = pService->createCharacteristic(
            CHAR_STATUS_UUID,
            BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY
        );
        pCharStatus->addDescriptor(new BLE2902());
        pCharStatus->setValue("Online");

        // Start Service
        pService->start();

        // Start Advertising with complete setup
        BLEAdvertising* pAdvertising = BLEDevice::getAdvertising();
        pAdvertising->addServiceUUID(SERVICE_UUID);
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMaxPreferred(0x12);
        
        // Add device name to advertising data (important!)
        pAdvertising->setAppearance(0x00);
        
        // Enable complete 16-bit UUID list in advertising
        pServer->getAdvertising()->start();

        initialized = true;
        Serial.printf(" BLE ready - advertising as '%s'...\n", deviceName);
        delay(500);
        return true;

    } catch (std::exception& e) {
        Serial.printf(" BLE init error: %s\n", e.what());
        return false;
    }
}

bool BluetoothManager::sendData(const char* data) {
    if (!initialized || !pCharTx) {
        Serial.println(" BLE not initialized");
        return false;
    }

    if (!connected) {
        Serial.println(" No BLE client connected");
        return false;
    }

    try {
        pCharTx->setValue((uint8_t*)data, strlen(data));
        pCharTx->notify();
        Serial.printf(" BLE TX: %s\n", data);
        return true;
    } catch (std::exception& e) {
        Serial.printf(" BLE send error: %s\n", e.what());
        return false;
    }
}

bool BluetoothManager::sendData(const uint8_t* data, size_t length) {
    if (!initialized || !pCharTx) {
        Serial.println(" BLE not initialized");
        return false;
    }

    if (!connected) {
        Serial.println(" No BLE client connected");
        return false;
    }

    try {
        pCharTx->setValue((uint8_t*)data, length);
        pCharTx->notify();
        Serial.printf(" BLE TX: %d bytes\n", length);
        return true;
    } catch (std::exception& e) {
        Serial.printf(" BLE send error: %s\n", e.what());
        return false;
    }
}

bool BluetoothManager::isConnected() const {
    return connected;
}

bool BluetoothManager::sendAudioChunk(const int16_t* samples, size_t sampleCount) {
    if (!initialized || !pCharTx) {
        return false;
    }

    if (!connected) {
        return false;
    }

    try {
        // Convert int16_t samples to bytes (max 512 bytes per BLE packet)
        // 512 bytes = 256 int16_t samples
        size_t maxSamples = 256;
        size_t samplesToSend = (sampleCount > maxSamples) ? maxSamples : sampleCount;
        size_t bytesToSend = samplesToSend * sizeof(int16_t);

        pCharTx->setValue((uint8_t*)samples, bytesToSend);
        pCharTx->notify();
        
        return true;
    } catch (std::exception& e) {
        return false;
    }
}

void BluetoothManager::updateStatus(const char* status) {
    if (!initialized || !pCharStatus) return;

    try {
        pCharStatus->setValue((uint8_t*)status, strlen(status));
        pCharStatus->notify();
        Serial.printf(" BLE Status: %s\n", status);
    } catch (std::exception& e) {
        Serial.printf(" BLE status update error: %s\n", e.what());
    }
}

void BluetoothManager::setOnDataReceived(DataReceivedCallback callback) {
    dataReceivedCallback = callback;
    Serial.println(" BLE data received callback registered");
}
