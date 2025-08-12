#include <Arduino.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"
#include "WebServerHandler.h"
#include "perdiavontadedeviver.h"
#include "SensorController.h"
#include "ActuatorController.h"

const char* AP_SSID = "IFungi-Config";
const char* AP_PASSWORD = "config1234";
String ifungiID = "IFUNGI-" + getMacAddress();
int acont6 = 0;
WiFiConfigurator wifiConfig;
FirebaseHandler firebase;
WebServerHandler webServer(wifiConfig, firebase);
SensorController sensors;
ActuatorController actuators;
void memateRapido();


void memateRapido(){
    String email, password;
    Serial.println("iniciando loop até loadFirebaseCredentials retornar true");
    while(!firebase.loadFirebaseCredentials(email,password)){
        Serial.print(".");
        webServer.begin(true); //ta conectado ao wifi entao pro firebase ficar ligado tem que ta true
    }
}


void setup() {
    Serial.begin(115200);
    delay(1000);
    // Defina os pinos corretos para seus componentes
    const uint8_t PIN_LED = 12;       // Exemplo
    const uint8_t PIN_RELE1 = 13;     // Exemplo
    const uint8_t PIN_RELE2 = 14;     // Exemplo
    const uint8_t PIN_RELE3 = 15;     // Exemplo
    const uint8_t PIN_RELE4 = 16;     // Exemplo
    // Inicializa sensores e atuadores
    sensors.begin();
    actuators.begin(PIN_LED, PIN_RELE1, PIN_RELE2, PIN_RELE3, PIN_RELE4);

    // Tenta conectar com WiFi salvo
    String ssid, password;
    if(wifiConfig.loadCredentials(ssid, password)) {
        if(wifiConfig.connectToWiFi(ssid.c_str(), password.c_str(), true)) {
            Serial.println("Conectado ao WiFi! Iniciando servidor...");
            webServer.begin(true);
            
            // Tenta autenticar com credenciais salvas
            String email, password;
            if(firebase.loadFirebaseCredentials(email, password)) {
                Serial.println("loadFirebase retournou verdade");
                firebase.authenticate(email, password);
            }else{
                Serial.println("loadFirebase retornou falso  iniciado a função do server do firebase sla");
                memateRapido();
                

            }
            return;
        }
    }

    // Modo AP se falhar
    wifiConfig.startAP(AP_SSID, AP_PASSWORD);
    webServer.begin(false);
}

void loop() {
    webServer.handleClient();
    if (firebase.isAuthenticated()) {
    static unsigned long lastSetpointCheck = 0;
    if (millis() - lastSetpointCheck > 30000) { // A cada 30 segundos
        firebase.RecebeSetpoint(actuators);
        lastSetpointCheck = millis();
    }
}
    // Atualiza sensores periodicamente
    
    sensors.update();
    while (acont6 <2) {
        Serial.println("Atualizando sensores...");
        Serial.println("Temperatura: " + String(sensors.getTemperature()) + " °C");
        acont6++;
    }
    

    
    // Processamento quando autenticado
    if (firebase.isAuthenticated()) {
        firebase.estufaExiste(firebase.estufaId);
        
        // Verifica/Cria estufa no Firebase
        static bool estufaVerified = false;
        if (!estufaVerified) {
            firebase.verificarEstufa();
            estufaVerified = true;
        }
        
        // Envia dados dos sensores para Firebase
        static unsigned long lastUpdate = 0;
        if(millis() - lastUpdate > 5000) { // A cada 5 segundos
            firebase.enviarDadosSensores(
                sensors.getTemperature(),
                sensors.getHumidity(),
                sensors.getCO2(),
                sensors.getCO(),
                sensors.getLight()
            );
            lastUpdate = millis();
        }
        
        // Recebe comandos do Firebase
        firebase.verificarComandos(actuators);
    }
    
    // Controle local dos atuadores baseado nos sensores
    actuators.controlarAutomaticamente(
        sensors.getTemperature(),
        sensors.getHumidity(),
        sensors.getLight()
    );
    
    delay(100);
}