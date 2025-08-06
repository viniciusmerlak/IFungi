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
        if(wifiConfig.connectToWiFi(ssid.c_str(), password.c_str())) {
            Serial.println("Conectado ao WiFi! Iniciando servidor...");
            webServer.begin(true); // Modo WiFi conectado
            return;
        }
    }

    // Modo AP se falhar
    wifiConfig.startAP(AP_SSID, AP_PASSWORD);
    webServer.begin(false); // Modo AP
}

void loop() {
    if (firebase.authenticated == false) {
        webServer.handleClient();
        delay(5);
    }

    firebase.seraQeuCrio();
    delay(5000);
    


}