#include <Arduino.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"
#include "WebServerHandler.h"
#include "SensorController.h"
#include "ActuatorController.h"
#include "perdiavontadedeviver.h"
const char* AP_SSID = "IFungi-Config";
const char* AP_PASSWORD = "config1234";
String ifungiID;
WiFiConfigurator wifiConfig;
FirebaseHandler firebase;
WebServerHandler webServer(wifiConfig, firebase);
SensorController sensors;
ActuatorController actuators;
void memateRapido();


void memateRapido(){
    String email, password;
    Serial.println("iniciando loop até loadFirebaseCredentials retornar true");
    while((email,password) != ("") && firebase.authenticated == true && !firebase.loadFirebaseCredentials(email,password)){
        Serial.print(".");
        webServer.begin(true); //ta conectado ao wifi entao pro firebase ficar ligado tem que ta true
    }
}


void setup() {
    Serial.begin(115200);
    delay(1000);
    
    // Inicializa sensores e atuadores
    sensors.begin();
    actuators.begin(4, 14, 23, 18, 19);
    
    // Configura setpoints padrão
    actuators.aplicarSetpoints(5000, 20.0, 30.0, 60.0, 80.0);

    // Tenta conectar com WiFi salvo
    String ssid, password;
    if(wifiConfig.loadCredentials(ssid, password)) {
        if(wifiConfig.connectToWiFi(ssid.c_str(), password.c_str(), true)) {
            Serial.println("Conectado ao WiFi! Iniciando servidor...");
            
            // Obtém o ID da estufa
            ifungiID = "IFUNGI-" + getMacAddress();
            
            webServer.begin(true);
            
            // Tenta autenticar com credenciais salvas
            String email, firebasePassword;
            if(firebase.loadFirebaseCredentials(email, firebasePassword)) {
                Serial.println("Credenciais do Firebase encontradas, autenticando...");
                if(firebase.authenticate(email, firebasePassword)) {
                    Serial.println("Autenticação bem-sucedida!");
                } else {
                    Serial.println("Falha na autenticação com credenciais salvas.");
                }
            } else {
                Serial.println("Nenhuma credencial do Firebase encontrada.");
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
    const unsigned long FIREBASE_INTERVAL = 5000;
    const unsigned long TOKEN_CHECK_INTERVAL = 300000;
    const unsigned long AUTH_RETRY_INTERVAL = 30000;

    webServer.handleClient();
    sensors.update();
    
    actuators.controlarAutomaticamente(
        sensors.getTemperature(),
        sensors.getHumidity(),
        sensors.getLight()
    );

    if(firebase.isAuthenticated()) {
        if(millis() - lastTokenCheck > TOKEN_CHECK_INTERVAL) {
            firebase.refreshToken(); // Método público agora
            lastTokenCheck = millis();
        }

        if(millis() - lastFirebaseUpdate > FIREBASE_INTERVAL) {
            if(Firebase.ready()) {
                firebase.enviarDadosSensores(
                    sensors.getTemperature(),
                    sensors.getHumidity(),
                    sensors.getCO2(),
                    sensors.getCO(),
                    sensors.getLight()
                );
                
                firebase.verificarComandos(actuators);
                firebase.RecebeSetpoint(actuators);
                
                lastFirebaseUpdate = millis();
            } else {
                Serial.println("Token do Firebase inválido, tentando renovar...");
                firebase.refreshToken(); // Método público agora
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
    
    delay(100);
}