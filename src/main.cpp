#include <Arduino.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"
#include "WebServerHandler.h"
#include "perdiavontadedeviver.h"

const char* AP_SSID = "IFungi-Config";
const char* AP_PASSWORD = "config1234";
String ifungiID = "IFUNGI-" + getMacAddress();

WiFiConfigurator wifiConfig;
FirebaseHandler firebase;
WebServerHandler webServer(wifiConfig, firebase);

void setup() {
    Serial.begin(115200);
    delay(1000);

    // Tenta conectar com WiFi salvo
    String ssid, password;
    if(wifiConfig.loadCredentials(ssid, password)) {
        if(wifiConfig.connectToWiFi(ssid.c_str(), password.c_str(), true)) {
            Serial.println("Conectado ao WiFi! Iniciando servidor...");
            webServer.begin(true);
            return;
        }
    }

    // Modo AP se falhar
    wifiConfig.startAP(AP_SSID, AP_PASSWORD);
    webServer.begin(false);
}

void loop() {
    static bool initialAuthAttempt = true;
    
    if (!firebase.isAuthenticated() && initialAuthAttempt) {
        String email, password;
        if (webServer.getStoredFirebaseCredentials(email, password)) {
            firebase.authenticate(email, password);
        }
        initialAuthAttempt = false;
    }
    
    webServer.handleClient();
    
    // Processamento adicional quando autenticado
    if (firebase.isAuthenticated()) {
        static bool estufaVerified = false;
        if (!estufaVerified) {
            firebase.seraQeuCrio();
            estufaVerified = true;
        }
    }
    
    delay(100);
}