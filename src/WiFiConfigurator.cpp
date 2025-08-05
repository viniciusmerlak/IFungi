#include "WiFiConfigurator.h"
Preferences preferences;
void WiFiConfigurator::startAP(const char* apSSID, const char* apPassword) {
    pinMode(LED_AP, OUTPUT);
    digitalWrite(LED_AP, LOW);  // Acende o LED
    
    WiFi.softAP(apSSID, apPassword);
    apActive = true;
    Serial.println("AP Started - LED azul ligado");
}
bool WiFiConfigurator::connectToWiFi(const char* ssid, const char* password) {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(ssid, password);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nConnected to WiFi!");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        return true;
    } else {
        Serial.println("\nFailed to connect to WiFi");
        return false;
    }
}

void WiFiConfigurator::stopAP() {
    setAPStatus(false);  // Reaproveita a lógica existente
    WiFi.softAPdisconnect(true);
    Serial.println("AP completamente desligado");
}

void WiFiConfigurator::showErrorPattern() {
    for(int i = 0; i < 3; i++) {
        digitalWrite(LED_AP, LOW);
        delay(200);
        digitalWrite(LED_AP, HIGH);
        delay(200);
    }
}

bool WiFiConfigurator::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}
bool WiFiConfigurator::reconnectWiFi() {
    if(WiFi.status() != WL_CONNECTED) {
        String ssid, password;
        if(loadCredentials(ssid, password)) {
            Serial.println("Tentando reconectar ao WiFi...");
            return connectToWiFi(ssid.c_str(), password.c_str());
        }
    }
    return false;
}
String WiFiConfigurator::getLocalIP() {
    return WiFi.localIP().toString();
}
void WiFiConfigurator::saveCredentials(const char* ssid, const char* password) {
    preferences.begin("wifi-creds", false);
    preferences.putString("ssid", ssid);
    preferences.putString("password", password);
    preferences.end();
    Serial.println("Credentials saved to flash");
}

bool WiFiConfigurator::loadCredentials(String &ssid, String &password) {
    preferences.begin("wifi-creds", true);
    
    if (preferences.isKey("ssid")) {
        ssid = preferences.getString("ssid", "");
        password = preferences.getString("password", "");
        preferences.end();
        
        if (ssid.length() > 0) {
            Serial.println("Loaded credentials from flash");
            return true;
        }
    }
    
    preferences.end();
    return false;
}

void WiFiConfigurator::setAPStatus(bool active) {
    apActive = active;
    digitalWrite(LED_AP, active ? LOW : HIGH);  // LOW acende, HIGH apaga
    Serial.printf("AP Status: %s - LED %s\n", 
                 active ? "Ativo" : "Inativo",
                 active ? "Ligado" : "Desligado");
}
void WiFiConfigurator::clearCredentials() {
    preferences.begin("wifi-creds", false);
    preferences.clear();
    preferences.end();
    Serial.println("Credentials cleared from flash");
}