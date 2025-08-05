#include "WiFiConfigurator.h"
#include "AuthManager.h"
#include "EstufaManager.h"
#include "DataSyncManager.h"
#include "AtuadoresManager.h"
#include "SensoresManager.h"

// Instâncias globais
WiFiConfigurator wifiConfigurator;
FirebaseData fbdo;  // Agora é uma instância normal
AuthManager authManager(fbdo);
EstufaManager estufaManager(fbdo);
DataSyncManager dataSyncManager(fbdo);  // Passa a referência
AtuadoresManager atuadoresManager(fbdo);
SensoresManager sensoresManager(4, DHT22);

String userUID;
String estufaID;


void setup() {
    Serial.begin(115200);
    
    // 1. Configuração WiFi
    String savedSSID, savedPassword;
    if(wifiConfigurator.loadCredentials(savedSSID, savedPassword)) {
        if(wifiConfigurator.connectToWiFi(savedSSID.c_str(), savedPassword.c_str())) {
            Serial.println("Conectado ao WiFi salvo");
        } else {
            wifiConfigurator.startAP("IFUNGI-CONFIG", "config123");
            Serial.println("Modo AP ativado");
            return;
        }
    } else {
        wifiConfigurator.startAP("IFUNGI-CONFIG", "config123");
        Serial.println("Modo AP ativado - Primeira configuração");
        return;
    }
    
    // 2. Autenticação (exemplo)
    if(authManager.authenticateUser("email@usuario.com", "senha123")) {
        userUID = authManager.getCurrentUser().uid;
        estufaID = "IFUNGI-" + WiFi.macAddress();
        
        // 3. Configuração da estufa
        if(!estufaManager.setupEstufa(estufaID, userUID)) {
            Serial.println("Falha na configuração da estufa");
            return;
        }
    }
}

void loop() {
    // Verifica conexão WiFi
    if(!WiFi.isConnected()) {
        Serial.println("Sem conexão WiFi - Tentando reconectar...");
        if(!wifiConfigurator.reconnectWiFi()) {
            delay(5000);
            return;
        }
    }
    
    // Leitura e sincronização
    SensoresManager::SensorReadings readings = sensoresManager.readSensors();
    dataSyncManager.syncSensorData(estufaID, readings);
    atuadoresManager.controlAtuadores(estufaID, readings);
    
    delay(10000);
}