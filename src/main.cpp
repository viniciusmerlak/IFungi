#include <Arduino.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"
#include "WebServerHandler.h"
#include "SensorController.h"
#include "ActuatorController.h"
#include "perdiavontadedeviver.h"
#include <nvs_flash.h>

const char* AP_SSID = "IFungi-Config";
const char* AP_PASSWORD = "config1234";

// Timeouts (em milissegundos)
const unsigned long WIFI_CONNECT_TIMEOUT = 30000;    // 30s para conectar ao WiFi
const unsigned long AP_MODE_TIMEOUT = 300000;        // 5 minutos em modo AP
const unsigned long FIREBASE_TIMEOUT = 60000;        // 1 minuto para autenticar Firebase
const unsigned long FIREBASE_INTERVAL = 5000;        // 5s entre atualizações
const unsigned long TOKEN_REFRESH_INTERVAL = 300000; // 5 minutos
const unsigned long SENSOR_UPDATE_INTERVAL = 2000;   // 2s entre leituras
const unsigned long AUTH_RETRY_INTERVAL = 300000; // 5 minutos entre tentativas de autenticação
WiFiConfigurator wifiConfig;
extern FirebaseHandler firebase;
WebServerHandler webServer(wifiConfig, firebase);
SensorController sensors;
ActuatorController actuators;
//Definindo funções globais
void controlBasicOperations();
void initNVS() {
    esp_err_t ret = nvs_flash_init();
    
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        Serial.println("[NVS] Particão corrompida - limpando...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    // Verificação opcional
    nvs_stats_t nvs_stats;
    if(nvs_get_stats(NULL, &nvs_stats) == ESP_OK) {
        Serial.printf("[NVS] Stats: %d/%d entradas usadas\n", 
                     nvs_stats.used_entries, nvs_stats.total_entries);
    }
}

void setupHardware() {
    Serial.begin(115200);
    delay(1000); // Estabilização
    Serial.println("\n\nInicializando sistema IFungi...");

    // Inicializa sensores e atuadores
    sensors.begin();
    actuators.begin(4, 14, 23, 18, 19); // Pinos fixos
    
    // Verificação crítica dos sensores
    if(!sensors.dhtOK || !sensors.ccsOK) {
        Serial.println("ERRO: Sensores críticos não inicializados!");
    }
    
    actuators.aplicarSetpoints(5000, 20.0, 30.0, 60.0, 80.0);
}

bool connectToWiFi() {
    String ssid, password;
    if(wifiConfig.loadCredentials(ssid, password)) {
        Serial.print("Tentando conectar ao WiFi salvo: ");
        Serial.println(ssid);
        
        unsigned long startTime = millis();
        while(millis() - startTime < WIFI_CONNECT_TIMEOUT) {
            if(wifiConfig.connectToWiFi(ssid.c_str(), password.c_str(), true)) {
                Serial.println("Conectado com sucesso!");
                return true;
            }
            delay(500);
        }
    }
    return false;
}
bool checkOrCreateNamespace(const char* namespaceName) {  // Renomeado para evitar conflito com palavra-chave
    Preferences preferences;
    
    // Tentativa 1: Abrir em modo leitura (verifica se existe)
    if(preferences.begin(namespaceName, true)) {
        Serial.printf("[NVS] Namespace '%s' aberto com sucesso (leitura)\n", namespaceName);
        preferences.end();
        return true;
    }
    
    Serial.printf("[NVS] Namespace '%s' não encontrado, tentando criar...\n", namespaceName);
    
    // Tentativa 2: Abrir em modo escrita (cria se não existir)
    if(preferences.begin(namespaceName, false)) {
        // Testa escrita/leitura para confirmar funcionamento
        const char* testKey = "test_key";
        const char* testValue = "test_value";
        
        if(!preferences.putString(testKey, testValue)) {
            Serial.println("[NVS ERRO] Falha ao escrever teste no namespace");
            preferences.end();
            return false;
        }
        
        if(preferences.getString(testKey, "") != String(testValue)) {
            Serial.println("[NVS ERRO] Falha ao ler teste do namespace");
            preferences.end();
            return false;
        }
        
        // Limpa o teste
        preferences.remove(testKey);
        preferences.end();
        
        Serial.printf("[NVS] Namespace '%s' criado e verificado com sucesso\n", namespaceName);
        return true;
    }
    
    Serial.printf("[NVS ERRO] Falha crítica ao criar namespace '%s'\n", namespaceName);
    return false;
}

bool authenticateFirebase() {
    String email, password;
    
    // Tentativa 1: Carrega credenciais salvas
    if(firebase.loadFirebaseCredentials(email, password)) {
        Serial.println("[INFO] Tentando autenticar com credenciais salvas...");
        if(firebase.authenticate(email, password)) {
            firebase.verificarEstufa();
            return true;
        }
    }
    
    // Tentativa 2: Modo configuração via WebServer
    webServer.begin(false); // Modo STA para config Firebase
    Serial.println("[INFO] Aguardando configuração Firebase via portal...");
    
    unsigned long fbStartTime = millis();
    while(millis() - fbStartTime < FIREBASE_TIMEOUT) {
        webServer.handleClient();
        controlBasicOperations();
        
        // Verifica se o servidor web recebeu novas credenciais
        if(webServer.getStoredFirebaseCredentials(email, password)) {
            if(firebase.authenticate(email, password)) {
                firebase.verificarEstufa();
                return true;
            }
        }
        delay(10);
    }
    
    return false;
}

void controlBasicOperations() {
    static unsigned long lastSensorUpdate = 0;
    
    if(millis() - lastSensorUpdate > SENSOR_UPDATE_INTERVAL) {
        sensors.update();
        actuators.controlarAutomaticamente(
            sensors.getTemperature(),
            sensors.getHumidity(),
            sensors.getLight()
        );
        lastSensorUpdate = millis();
    }
}
void setup() {
    // 1. Inicialização crítica
    Serial.begin(115200);
    delay(1000); // Estabilização
    Serial.println("\n\nInicializando sistema IFungi...");

    // 2. Inicializa NVS
    initNVS();
    
    // 3. Configura hardware
    setupHardware();
    actuators.setFirebaseHandler(&firebase);

    // 4. Conexão WiFi
    if(!connectToWiFi()) {
        wifiConfig.startAP(AP_SSID, AP_PASSWORD);
        webServer.begin(true);
        
        unsigned long apStartTime = millis();
        while(millis() - apStartTime < AP_MODE_TIMEOUT) {
            webServer.handleClient();
            controlBasicOperations();
            delay(10);
        }
        ESP.restart();
    }

    // 5. Autenticação Firebase
    if(!authenticateFirebase()) {
        webServer.begin(false); // Modo STA para config Firebase
        Serial.println("Aguardando configuração Firebase via portal...");
        
        unsigned long fbStartTime = millis();
        while(millis() - fbStartTime < FIREBASE_TIMEOUT) {
            webServer.handleClient();
            controlBasicOperations();
            delay(10);
        }
        ESP.restart();
    }

    Serial.println("Sistema inicializado com sucesso!");
}

void loop() {
    static unsigned long lastErrorReport = 0;
    
    // 1. Controle básico
    controlBasicOperations();

    // 2. Gerenciamento do servidor web
    webServer.handleClient();

    // 3. Lógica do Firebase (apenas se autenticado)
    if(firebase.isAuthenticated()) {
        static unsigned long lastUpdate = 0;
        if(millis() - lastUpdate > FIREBASE_INTERVAL) {
            if(Firebase.ready()) {
                firebase.enviarDadosSensores(
                    sensors.getTemperature(),
                    sensors.getHumidity(),
                    sensors.getCO2(),
                    sensors.getCO(),
                    sensors.getLight()
                );
                lastUpdate = millis();
            }
        }
    }
    
    // 4. Reporte periódico de erros (a cada 30s)
    if(millis() - lastErrorReport > 30000) {
        if(!sensors.dhtOK) Serial.println("[ERRO] DHT22 não está respondendo");
        if(!sensors.ccsOK) Serial.println("[ERRO] CCS811 não está respondendo");
        lastErrorReport = millis();
    }
    
    delay(10);
}