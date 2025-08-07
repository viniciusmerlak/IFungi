#ifndef WEBSERVER_HANDLER_H
#define WEBSERVER_HANDLER_H

#include <WebServer.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"
#include <Preferences.h>

class WebServerHandler {
public:
    WebServerHandler(WiFiConfigurator& wifiConfig, FirebaseHandler& fbHandler);
    void begin(bool wifiConnected);
    void handleClient();
    bool getStoredFirebaseCredentials(String& email, String& password);
    void handleResetAuth(); // Adicionado
    

private:
    WebServer server;
    WiFiConfigurator& wifiConfigurator;
    FirebaseHandler& firebaseHandler;
    bool wifiConnected;
    const String FIREBASE_API_KEY = "AIzaSyDkPzzLHykaH16FsJpZYwaNkdTuOOmfnGE";
    const String DATABASE_URL = "pfi-ifungi-default-rtdb.firebaseio.com";
    
    void saveFirebaseCredentials(const String& email, const String& password);
    bool loadFirebaseCredentials(String& email, String& password);
    void handleRoot();
    void handleWiFiConfig();
    void handleFirebaseConfig();
    void handleNotFound();
};

#endif