#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <WiFi.h>
#include <Arduino.h>
#include <WebServer.h>
#include <Preferences.h>

class WiFiManager {
public:
  WiFiManager(const char* fallback_ssid, const char* fallback_password);

  void begin();
  bool isConnected();
  String getIP();
  int getRSSI();
  void reconnect();

  // AP and WebServer methods
  void startAPServer();
  void handleClient();
  void loadCredentials();
  
  // HTML Content for the root path
  String getHtmlPage();

private:
  const char* _fallback_ssid;
  const char* _fallback_password;

  String _current_ssid;
  String _current_password;

  WebServer server;
  Preferences preferences;
  
  void handleRoot();
  void handleSave();
};

#endif
