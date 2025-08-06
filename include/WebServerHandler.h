#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <WebServer.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"

class WebServerHandler {
public:
    WebServerHandler(WiFiConfigurator& wifiConfig, FirebaseHandler& fbHandler);
    void begin(bool wifiConnected);
    void handleClient();
    
private:
    WebServer server;
    WiFiConfigurator& wifiConfigurator;
    FirebaseHandler& firebaseHandler;
    bool wifiConnected;
    
    void handleRoot();
    void handleWiFiConfig();
    void handleFirebaseConfig();
    void handleNotFound();
};

#endif