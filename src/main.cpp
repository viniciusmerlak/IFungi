#include <Arduino.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"
#include "WebServerHandler.h"
#include "SensorController.h"
#include "ActuatorController.h"
#include "perdiavontadedeviver.h"
#include "genQrCode.h"  // Corrigido o nome do arquivo

const char* AP_SSID = "IFungi-Config";
const char* AP_PASSWORD = "config1234";
String ifungiID;
WiFiConfigurator wifiConfig;
FirebaseHandler firebase;
WebServerHandler webServer(wifiConfig, firebase);
GenQR qrcode;
SensorController sensors;
ActuatorController actuators;

void memateRapido() {
    String email, password;
    Serial.println("iniciando loop até loadFirebaseCredentials retornar true");
    while((email, password) != ("") && firebase.authenticated == true && !firebase.loadFirebaseCredentials(email, password)) {
        Serial.print(".");
        webServer.begin(true);
    }
}
void ledes(){
    if(firebase.isAuthenticated() && wifiConfig.isConnected()) {
        wifiConfig.piscaLED(true, 666666); // mantem ligado se tudo estiver ok
        return; // Se tudo estiver ok, não faz nada
    }
    if(firebase.isAuthenticated() && wifiConfig.isConnected()) {
        wifiConfig.piscaLED(true, 200); // pisca rapido
    }else if(!wifiConfig.isConnected()) {
        wifiConfig.piscaLED(true, 666666); // pisca rápido se não estiver conectado no wifi
    } else if(!firebase.isAuthenticated()) {
        wifiConfig.piscaLED(true, 777777); // pisca em pulso
    } else {
        wifiConfig.piscaLED(false, 0); // led apagado se estiver tudo ok
    }
}
void setup() {
    Serial.begin(115200);
    delay(1000);
    firebase.setWiFiConfigurator(&wifiConfig);
    
    // Inicializa sensores e atuadores
    sensors.begin();
    actuators.begin(4, 23, 14, 18, 19);
    
    // Tenta carregar setpoints do NVS, se não existir, usa os padrões
    if(!actuators.carregarSetpointsNVS()) {
        // Configura setpoints padrão
        actuators.aplicarSetpoints(5000, 20.0, 30.0, 60.0, 80.0, 400, 400, 100);
    }

    // Tenta conectar com WiFi salvo
    String ssid, password;
    if(wifiConfig.loadCredentials(ssid, password)) {
        if(wifiConfig.connectToWiFi(ssid.c_str(), password.c_str(), true)) {
            Serial.println("Conectado ao WiFi! Iniciando servidor...");
            ledes();
            // Obtém o ID da estufa
            ifungiID = "IFUNGI-" + getMacAddress();
            Serial.println("ID da Estufa: " + ifungiID);
            qrcode.generateQRCode(ifungiID); // Gera QR code com o ID real da estufa
            
            webServer.begin(true);
            
            // Tenta autenticar com credenciais salvas
            String email, firebasePassword;
            ledes();
            if(firebase.loadFirebaseCredentials(email, firebasePassword)) {
                Serial.println("Credenciais do Firebase encontradas, autenticando...");
                
                if(firebase.authenticate(email, firebasePassword)) {
                    Serial.println("Autenticação bem-sucedida!");
                    ledes();
                } else {
                    Serial.println("Falha na autenticação com credenciais salvas.");
                    ledes();
                }
            } else {
                Serial.println("Nenhuma credencial do Firebase encontrada.");
                ledes();
            }
        } else {
            wifiConfig.startAP(AP_SSID, AP_PASSWORD);
            webServer.begin(false);
            ledes();
        }
        ledes();
    } else {
        wifiConfig.startAP(AP_SSID, AP_PASSWORD);
        webServer.begin(false);
        ledes();
    }
    actuators.setFirebaseHandler(&firebase);
    ledes();
}

void loop() {
    ledes();
    static unsigned long lastFirebaseUpdate = 0;
    static unsigned long lastTokenCheck = 0;
    static unsigned long lastAuthAttempt = 0;
    static unsigned long lastSensorRead = 0;
    static unsigned long lastActuatorControl = 0;
    static unsigned long lastHeartbeat = 0; // Novo
    
    const unsigned long FIREBASE_INTERVAL = 5000;
    const unsigned long TOKEN_CHECK_INTERVAL = 300000;
    const unsigned long AUTH_RETRY_INTERVAL = 30000;
    const unsigned long SENSOR_READ_INTERVAL = 2000;
    const unsigned long ACTUATOR_CONTROL_INTERVAL = 5000;
    const unsigned long HEARTBEAT_INTERVAL = 15000; // 30 segundos

    // 1. Gerencia o servidor web
    webServer.handleClient();
    
    // 2. Atualiza leituras dos sensores
    if (millis() - lastSensorRead > SENSOR_READ_INTERVAL) {
        sensors.update();
        lastSensorRead = millis();
    }
    
    // 3. Controle automático dos atuadores
    if (millis() - lastActuatorControl > ACTUATOR_CONTROL_INTERVAL) {
        actuators.controlarAutomaticamente(
            sensors.getTemperature(),
            sensors.getHumidity(),
            sensors.getLight(),
            sensors.getCO(),
            sensors.getCO2(),
            sensors.getTVOCs(),
            sensors.getWaterLevel() // Adicionado o sensor de nível de água
        );
        lastActuatorControl = millis();
    }
    // 4. Enviar heartbeat periodicamente
    if (millis() - lastHeartbeat > HEARTBEAT_INTERVAL) {
        if (firebase.isAuthenticated()) {
            firebase.enviarHeartbeat();
        }
        lastHeartbeat = millis();
    }
    // 4. Lógica do Firebase (mantida como antes)
    if(firebase.isAuthenticated()) {
        if(millis() - lastTokenCheck > TOKEN_CHECK_INTERVAL) {
            firebase.refreshToken();
            lastTokenCheck = millis();
        }

        if(millis() - lastFirebaseUpdate > FIREBASE_INTERVAL) {
            if(Firebase.ready()) {
                firebase.enviarDadosSensores(
                    sensors.getTemperature(),
                    sensors.getHumidity(),
                    sensors.getCO2(),
                    sensors.getCO(),
                    sensors.getLight(),
                    sensors.getTVOCs(), // Add this
                    sensors.getWaterLevel() // Adicionado o sensor de nível de água
                );
                
                firebase.verificarComandos(actuators);
                firebase.RecebeSetpoint(actuators);
                
                lastFirebaseUpdate = millis();
            } else {
                Serial.println("Token do Firebase inválido, tentando renovar...");
                firebase.refreshToken();
            }
        }
    } else {
        if(millis() - lastAuthAttempt > AUTH_RETRY_INTERVAL) {
            String email, password;
            if(firebase.loadFirebaseCredentials(email, password)) {
                if(firebase.authenticate(email, password)) {
                    firebase.verificarEstufa();
                }
            }
            lastAuthAttempt = millis();
        }
    }
    
    // Pequena pausa não-bloqueante para estabilidade
    delay(10);
}