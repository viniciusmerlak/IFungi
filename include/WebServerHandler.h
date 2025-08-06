#ifndef WEB_SERVER_HANDLER_H
#define WEB_SERVER_HANDLER_H

#include <WebServer.h>
#include "WiFiConfigurator.h"
#include "FirebaseHandler.h"
#include <Preferences.h>  
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
    String FIREBASE_API_KEY = "AIzaSyDkPzzLHykaH16FsJpZYwaNkdTuOOmfnGE";
    String DATABASE_URL = "pfi-ifungi-default-rtdb.firebaseio.com";
    void saveFirebaseCredentials(const String& email, const String& password);
    void handleRoot();
    void handleWiFiConfig();
    void handleFirebaseConfig();
    void handleNotFound();
    bool loadFirebaseCredentials(String& email, String& password);
};

#endif