#ifndef BLUETOOTH_MANAGER_H
#define BLUETOOTH_MANAGER_H

#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

// UUID Definitions
#define SERVICE_UUID        "12345678-1234-1234-1234-123456789012"
#define CHAR_TX_UUID        "12345678-1234-1234-1234-123456789013"  // TX to client
#define CHAR_RX_UUID        "12345678-1234-1234-1234-123456789014"  // RX from client
#define CHAR_STATUS_UUID    "12345678-1234-1234-1234-123456789015"  // Device status

class BluetoothManager {
public:
    BluetoothManager();
    ~BluetoothManager();

    // Initialize Bluetooth
    bool begin(const char* deviceName = "ESP32-IoT-Device");

    // Send data to connected BLE client
    bool sendData(const char* data);
    bool sendData(const uint8_t* data, size_t length);

    // Send audio chunk (for streaming audio)
    bool sendAudioChunk(const int16_t* samples, size_t sampleCount);

    // Get connection status
    bool isConnected() const;

    // Update status characteristic
    void updateStatus(const char* status);

    // Callback function type for received data
    typedef void (*DataReceivedCallback)(const uint8_t* data, size_t length);

    // Set callback for data reception
    void setOnDataReceived(DataReceivedCallback callback);

private:
    BLEServer* pServer;
    BLEService* pService;
    BLECharacteristic* pCharTx;
    BLECharacteristic* pCharRx;
    BLECharacteristic* pCharStatus;
    bool initialized;
    bool connected;
    DataReceivedCallback dataReceivedCallback;

    friend class ServerCallbacks;
    friend class CharacteristicCallbacks;
};

#endif
