#include "WiFiConfigurator.h"

void WiFiConfigurator::startAP(const char* apSSID, const char* apPassword) {
    WiFi.softAP(apSSID, apPassword);
    Serial.println("AP Started");
    Serial.print("SSID: "); Serial.println(apSSID);
    Serial.print("IP: "); Serial.println(WiFi.softAPIP());
}

bool WiFiConfigurator::connectToWiFi(const char* ssid, const char* password) {
    Serial.println("Connecting to WiFi...");
    Serial.print("Conectando-se a ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);
    
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("");
    Serial.println("WiFi conectada.");
    Serial.println("Endereço de IP: ");
    Serial.println(WiFi.localIP());

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected!");
        Serial.print("IP: "); Serial.println(WiFi.localIP());
        return true;
    }
    
    Serial.println("\nConnection failed");
    return false;
}

void WiFiConfigurator::stopAP() {
    WiFi.softAPdisconnect(true);
    Serial.println("AP stopped");
}

bool WiFiConfigurator::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiConfigurator::getLocalIP() {
    return WiFi.localIP().toString();
}

bool WiFiConfigurator::loadCredentials(String &ssid, String &password) {
    if(!preferences.begin("wifi-creds", true)) { // Modo read-only
        Serial.println("Namespace não existe, criando...");
        preferences.end();
        return false;
    }
    
    if(preferences.isKey("ssid")) {
        ssid = preferences.getString("ssid", "");
        password = preferences.getString("password", "");
        preferences.end();
        return ssid.length() > 0;
    }
    preferences.end();
    return false;
}

void WiFiConfigurator::saveCredentials(const char* ssid, const char* password) {
    if(!preferences.begin("wifi-creds", false)) { // Modo read-write
        Serial.println("Falha ao acessar NVS!");
        return;
    }
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    Serial.println("Credenciais salvas no NVS");
}

void WiFiConfigurator::clearCredentials() {
    preferences.begin("wifi-creds", false);
    preferences.clear();
    preferences.end();
    nvs_flash_erase(); // Apaga toda a partição NVS
    nvs_flash_init();  // Reinicializa
}