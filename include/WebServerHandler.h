#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <WebServer.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"

class WebServerHandler {
public:
    WebServerHandler(WiFiConfigurator& wifiConfig, FirebaseHandler& fbHandler);
    void begin();
    void handleClient();
    bool isConfigured();
    
    // Adicione estas declarações
    String getSSID();
    String getPassword();

private:
    WebServer server;
    WiFiConfigurator& wifiConfigurator;
    FirebaseHandler& firebaseHandler;
    bool configured;
    String ssid;
    String password;
    
    void handleRoot();
    void handleConfigure();
    void handleFirebaseConfig();
    void handleNotFound();
};

#endif