#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Arduino.h>

class WiFiManager {
public:
  WiFiManager(const char* ssid, const char* password);

  void begin();
  bool isConnected();
  String getIP();
  int getRSSI();
  void reconnect();

private:
  const char* _ssid;
  const char* _password;
};

#endif
