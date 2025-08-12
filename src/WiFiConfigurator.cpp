#include "WiFiConfigurator.h"

// Tempo máximo de tentativa de conexão (30 segundos)
const unsigned long WIFI_CONNECT_TIMEOUT = 30000;

// Tamanho máximo para SSID e password
const size_t MAX_SSID_LENGTH = 32;
const size_t MAX_PASSWORD_LENGTH = 64;

WiFiConfigurator::WiFiConfigurator() {
    preferences.begin("wifi-creds", false); // Inicia em modo RW
}

bool WiFiConfigurator::startAP(const char* apSSID, const char* apPassword) {
    WiFi.disconnect(true);  // Desconecta primeiro
    delay(100);
    WiFi.mode(WIFI_AP);
    
    // Configuração mais robusta do AP
    WiFi.softAPConfig(
        IPAddress(192, 168, 4, 1),
        IPAddress(192, 168, 4, 1),
        IPAddress(255, 255, 255, 0));
    
    bool result = WiFi.softAP(apSSID, apPassword, 6, 0, 4); // Canal 6, SSID visível, 4 conexões
    
    if(result) {
        Serial.printf("[WiFi] AP OK - SSID: %s - IP: ", apSSID);
        Serial.println(WiFi.softAPIP());
        return true;
    }
    Serial.println("[WiFi] Falha ao criar AP");
    return false;
}

bool WiFiConfigurator::connectToWiFi(const char* ssid, const char* password, bool persistent) {
    if(!ssid || strlen(ssid) == 0) {
        Serial.println("[WiFi] SSID inválido");
        return false;
    }

    Serial.printf("[WiFi] Conectando a: %s\n", ssid);
    
    // Configuração otimizada
    WiFi.disconnect(true);
    delay(100);
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    WiFi.persistent(persistent);
    
    WiFi.begin(ssid, password);
    
    unsigned long startTime = millis();
    while (WiFi.status() != WL_CONNECTED && 
          (millis() - startTime < WIFI_CONNECT_TIMEOUT)) {
        delay(250);
        Serial.print(".");
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Conectado!\nIP: %s\n", WiFi.localIP().toString().c_str());
        if(persistent) {
            saveCredentials(ssid, password);
        }
        return true;
    }
    
    Serial.println("\n[WiFi] Falha na conexão");
    WiFi.disconnect();
    return false;
}

bool WiFiConfigurator::loadCredentials(String &ssid, String &password) {
    if(!preferences.begin("wifi-creds", true)) { // Modo RO
        Serial.println("[NVS] Namespace não encontrado");
        return false;
    }
    
    ssid = preferences.getString("ssid", "");
    password = preferences.getString("password", "");
    preferences.end();
    
    if(ssid.length() == 0) {
        Serial.println("[NVS] Nenhum SSID salvo");
        return false;
    }
    
    Serial.printf("[NVS] Credenciais carregadas - SSID: %s\n", ssid.c_str());
    return true;
}

bool WiFiConfigurator::saveCredentials(const char* ssid, const char* password) {
    preferences.begin("wifi-creds", false); // Modo RW
    
    bool success = true;
    if(!preferences.putString("ssid", ssid)) {
        Serial.println("[ERRO] Falha ao salvar SSID");
        success = false;
    }
    
    if(!preferences.putString("password", password ? password : "")) {
        Serial.println("[ERRO] Falha ao salvar senha");
        success = false;
    }
    
    preferences.end();
    
    if(success) {
        Serial.printf("[NVS] Credenciais salvas - SSID: %s\n", ssid);
        // Confirma que foi salvo
        String savedSsid = preferences.getString("ssid", "");
        Serial.printf("[NVS] Verificação: %s\n", savedSsid.c_str());
    }
    
    return success;
}

void WiFiConfigurator::clearCredentials() {
    preferences.clear();
    Serial.println("[WiFi] Credenciais apagadas");
}

bool WiFiConfigurator::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

String WiFiConfigurator::getLocalIP() {
    return WiFi.localIP().toString();
}

void WiFiConfigurator::reconnectOrFallbackToAP(const char* apSSID, const char* apPassword) {
    String ssid, password;
    if(loadCredentials(ssid, password)) {
        Serial.printf("[WiFi] Tentando reconectar a: %s\n", ssid.c_str());
        if(connectToWiFi(ssid.c_str(), password.c_str(), false)) {
            return;
        }
    }
    
    Serial.println("[WiFi] Iniciando modo AP");
    startAP(apSSID, apPassword);
}