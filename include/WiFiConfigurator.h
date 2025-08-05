#ifndef WIFI_CONFIGURATOR_H
#define WIFI_CONFIGURATOR_H

#include <WiFi.h>
#include <Preferences.h>

class WiFiConfigurator {
public:
    void startAP(const char* apSSID, const char* apPassword = nullptr);
    void setAPStatus(bool active);  // Método existente - vamos implementá-lo
    bool connectToWiFi(const char* ssid, const char* password);
    void stopAP();
    bool isConnected();
    String getLocalIP();
    
    // Métodos para Preferences (mantidos)
    void saveCredentials(const char* ssid, const char* password);
    bool loadCredentials(String &ssid, String &password);
    void clearCredentials();
    void showErrorPattern();  // Novo método para mostrar padrão de erro
    bool reconnectWiFi();
    // Novo método para verificar estado do AP
    bool isAPActive() const { return apActive; }  // Implementação inline

private:
    const int LED_AP = 2;  // GPIO2 para o LED azul
    bool apActive = false;
    Preferences preferences;
};

#endif