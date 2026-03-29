#include "WiFiManager.h"

WiFiManager::WiFiManager(const char* fallback_ssid, const char* fallback_password) 
  : server(80) 
{
  _fallback_ssid = fallback_ssid;
  _fallback_password = fallback_password;
}

void WiFiManager::loadCredentials() {
  preferences.begin("wifi", true); // true for read-only
  _current_ssid = preferences.getString("ssid", _fallback_ssid);
  _current_password = preferences.getString("password", _fallback_password);
  preferences.end();
}

void WiFiManager::begin() {
  loadCredentials();

  WiFi.mode(WIFI_STA);
  WiFi.begin(_current_ssid.c_str(), _current_password.c_str());

  Serial.printf("[WiFi] Connecting to: %s\n", _current_ssid.c_str());

  int retry = 0;
  while (WiFi.status() != WL_CONNECTED && retry < 20) {
    delay(500);
    Serial.print(".");
    retry++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] Connected!");
    Serial.print("[WiFi] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\n[WiFi] Failed to connect.");
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
    WiFi.begin(_current_ssid.c_str(), _current_password.c_str());
  }
}

// ================= AP & WebServer =================

void WiFiManager::startAPServer() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP("ESP32-Setup");
  
  Serial.println("[AP] Started with SSID: ESP32-Setup");
  Serial.print("[AP] IP Address: ");
  Serial.println(WiFi.softAPIP());

  server.on("/", HTTP_GET, [this]() { this->handleRoot(); });
  server.on("/save", HTTP_POST, [this]() { this->handleSave(); });
  server.on("/save", HTTP_GET, [this]() { this->handleSave(); }); // In case simple GET form is used
  
  server.begin();
  Serial.println("[WebServer] Started");
}

void WiFiManager::handleClient() {
  server.handleClient();
}

void WiFiManager::handleRoot() {
  server.send(200, "text/html", getHtmlPage());
}

void WiFiManager::handleSave() {
  if (server.hasArg("ssid") && server.hasArg("password")) {
    String ssid = server.arg("ssid");
    String pass = server.arg("password");

    preferences.begin("wifi", false); // false for read-write
    preferences.putString("ssid", ssid);
    preferences.putString("password", pass);
    preferences.end();

    String successMsg = "<html><body><div style='font-family: Arial; text-align: center; margin-top: 50px;'><h1>Saved!</h1><p>Restarting ESP32 to apply...</p></div></body></html>";
    server.send(200, "text/html", successMsg);
    
    Serial.println("[WebServer] Credentials saved. Restarting...");
    delay(3000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing ssid or password");
  }
}

String WiFiManager::getHtmlPage() {
  String html = "<!DOCTYPE html><html><head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; display: flex; justify-content: center; align-items: center; height: 100vh; background-color: #f0f2f5; margin: 0;}";
  html += ".card { background: white; padding: 30px; border-radius: 10px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); width: 100%; max-width: 400px; text-align: center; box-sizing: border-box; }";
  html += "input[type=\"text\"], input[type=\"password\"] { width: 100%; padding: 12px 20px; margin: 8px 0; display: inline-block; border: 1px solid #ccc; box-sizing: border-box; border-radius: 4px; }";
  html += "button { background-color: #4CAF50; color: white; padding: 14px 20px; margin: 20px 0 8px 0; border: none; cursor: pointer; width: 100%; border-radius: 4px; font-size: 16px; font-weight: bold; }";
  html += "button:hover { opacity: 0.8; }";
  html += "</style></head><body>";
  
  html += "<div class=\"card\">";
  html += "<h2>WiFi Config</h2>";
  html += "<form action=\"/save\" method=\"POST\">";
  html += "<input type=\"text\" name=\"ssid\" placeholder=\"WiFi SSID\" required>";
  html += "<input type=\"password\" name=\"password\" placeholder=\"Password\">";
  html += "<button type=\"submit\">Save & Restart</button>";
  html += "</form>";
  html += "</div>";
  
  html += "</body></html>";
  return html;
}
