#ifndef WIFI_CONFIGURATOR_H
#define WIFI_CONFIGURATOR_H

#include <WiFi.h>
#include <Preferences.h>
#include <nvs_flash.h>
class WiFiConfigurator {
public:
    void startAP(const char* apSSID, const char* apPassword = nullptr);
    bool connectToWiFi(const char* ssid, const char* password);
    void stopAP();
    bool isConnected();
    String getLocalIP();
    
    void saveCredentials(const char* ssid, const char* password);
    bool loadCredentials(String &ssid, String &password);
    void clearCredentials();

private:
    Preferences preferences;
};

#endif