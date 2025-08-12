#ifndef WIFI_CONFIGURATOR_H
#define WIFI_CONFIGURATOR_H

#include <WiFi.h>
#include <Preferences.h>

class WiFiConfigurator {
public:
    WiFiConfigurator();
    
    bool startAP(const char* apSSID, const char* apPassword = nullptr);
    bool connectToWiFi(const char* ssid, const char* password = nullptr, bool persistent = true);
    void reconnectOrFallbackToAP(const char* apSSID, const char* apPassword = nullptr);
    
    bool loadCredentials(String &ssid, String &password);
    bool saveCredentials(const char* ssid, const char* password = nullptr);
    void clearCredentials();
    
    bool isConnected();
    String getLocalIP();

private:
    Preferences preferences;
};

#endif