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
    webServer.handleClient();  // Sempre processa requisições (importante!)
    
    if(!firebase.authenticated) {
        delay(5);  // Delay curto no modo AP
    } else {
        // Código após autenticação
        static bool estufaCriada = false;
        if(!estufaCriada) {
            firebase.seraQeuCrio();
            firebase.criarEstufaInicial("usuarioCriador", "usuarioAtual");
            estufaCriada = true;
        }
        delay(10000);  // Delay longo após autenticação
    }
}