#include <Arduino.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"
#include "WebServerHandler.h"
#include "SensorController.h"
#include "ActuatorController.h"
#include "perdiavontadedeviver.h"
const char* AP_SSID = "IFungi-Config";
const char* AP_PASSWORD = "config1234";

WiFiConfigurator wifiConfig;
FirebaseHandler firebase;
WebServerHandler webServer(wifiConfig, firebase);
SensorController sensors;
ActuatorController actuators;
String ifungiID;

void setup() {

    Serial.begin(115200);
    delay(1000);
    
    // Inicializa sensores e atuadores
    sensors.begin();
    actuators.begin(4, 14, 23, 18, 19); // Pinos fixos
    
    // Configura setpoints padrão (importante para quando não há Firebase)
    actuators.aplicarSetpoints(5000, 20.0, 30.0, 60.0, 80.0);

    // Tenta conectar com WiFi salvo
    String ssid, password;
    if(wifiConfig.loadCredentials(ssid, password)) {
        if(wifiConfig.connectToWiFi(ssid.c_str(), password.c_str(), true)) {
            Serial.println("Conectado ao WiFi! Iniciando servidor...");
            
            // Obtém o ID da estufa após inicialização
            ifungiID = "IFUNGI-" + getMacAddress();
            
            webServer.begin(true);
            String email, fbPassword;
            if(firebase.loadFirebaseCredentials(email, fbPassword)) {
                if(!firebase.authenticate(email, fbPassword)) {
                    Serial.println("Credenciais do Firebase inválidas");
                }else {
                    Serial.println('LEEEEEEEEEEEEEEPOOOOOOOOOOOOOOOOOO');
                    firebase.verificarEstufa();
                }
            }
        } else {
            wifiConfig.startAP(AP_SSID, AP_PASSWORD);
            webServer.begin(false);
        }
    } else {
        wifiConfig.startAP(AP_SSID, AP_PASSWORD);
        webServer.begin(false);
    }
    actuators.setFirebaseHandler(&firebase);
}
void loop() {
    static unsigned long lastFirebaseUpdate = 0;
    static unsigned long lastTokenCheck = 0;
    static unsigned long lastAuthAttempt = 0;
    const unsigned long FIREBASE_INTERVAL = 5000;   // 5 segundos
    const unsigned long TOKEN_CHECK_INTERVAL = 300000; // 5 minutos
    const unsigned long AUTH_RETRY_INTERVAL = 30000;  // 30 segundos

    // 1. Gerencia o servidor web
    webServer.handleClient();
    
    // 2. Atualiza leituras dos sensores
    sensors.update();
    
    // 3. Controle local independente (sempre ativo)
    actuators.controlarAutomaticamente(
        sensors.getTemperature(),
        sensors.getHumidity(),
        sensors.getLight()
    );

    // 4. Lógica do Firebase
    if(firebase.isAuthenticated()) {
        // Verifica/renova token periodicamente
        if(millis() - lastTokenCheck > TOKEN_CHECK_INTERVAL) {
            firebase.refreshTokenIfNeeded();
            lastTokenCheck = millis();
        }

        // Operações com Firebase (a cada 5 segundos)
        if(millis() - lastFirebaseUpdate > FIREBASE_INTERVAL) {
            if(Firebase.ready()) { // Verifica se o token está válido
                // Envia dados dos sensores (não é booleano, então removemos a verificação)
                firebase.enviarDadosSensores(
                    sensors.getTemperature(),
                    sensors.getHumidity(),
                    sensors.getCO2(),
                    sensors.getCO(),
                    sensors.getLight()
                );
                
                // Verifica comandos remotos
                firebase.verificarComandos(actuators);
                
                // Atualiza setpoints
                firebase.RecebeSetpoint(actuators);
                
                lastFirebaseUpdate = millis();
            } else {
                Serial.println("Token do Firebase inválido, tentando renovar...");
                firebase.refreshTokenIfNeeded();
            }
        }
    } 
    else {
        // Tenta autenticar periodicamente
        if(millis() - lastAuthAttempt > AUTH_RETRY_INTERVAL) {
            String email, password;
            if(firebase.loadFirebaseCredentials(email, password)) {
                if(firebase.authenticate(email, password)) {
                    // Se autenticou com sucesso, verifica/cria estufa
                    firebase.verificarEstufa();
                }
            }
            lastAuthAttempt = millis();
        }
    }
    
    // Pequena pausa para estabilidade
    delay(100);
}