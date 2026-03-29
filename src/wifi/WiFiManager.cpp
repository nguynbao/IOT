#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* ssid, const char* password) {
  _ssid = ssid;
  _password = password;
}

void WiFiManager::begin() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(_ssid, _password);

  Serial.print(" WiFi connecting");

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n WiFi connected");
    Serial.print("IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n WiFi failed (offline mode)");
  }
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

String WiFiManager::getIP() {
  if (isConnected())
    return WiFi.localIP().toString();
  return "No IP";
}

int WiFiManager::getRSSI() {
  if (isConnected())
    return WiFi.RSSI();
  return 0;
}

void WiFiManager::reconnect() {
  if (!isConnected()) {
    WiFi.disconnect();
    WiFi.begin(_ssid, _password);
  }
}
