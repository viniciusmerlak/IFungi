#include "FirebaseHandler.h"
#include "WebServer.h"


void FirebaseHandler::begin(const String &FIREBASE_API_KEY, const String &FIREBASE_EMAIL, const String &FIREBASE_PASSWORD) {
    config.api_key = FIREBASE_API_KEY.c_str();
    auth.user.email = FIREBASE_EMAIL.c_str();
    auth.user.password = FIREBASE_PASSWORD.c_str();
    Firebase.reconnectNetwork(true);

    Serial.println("Inicializando Firebase...");
    Firebase.begin(&config, &auth);
    

    // Espera com timeout
    int attempts = 0;
    while (!Firebase.ready() && attempts < 10) {
        delay(500);
        Serial.print(".");
        if (!Firebase.ready() && MB_String(auth.token.uid).length() == 0) {
            Serial.println("\nErro de autenticação: UID não recebido.");
            authenticated = false;
            return;
        }
        attempts++;
    }

    authenticated = Firebase.ready();
    Serial.println(authenticated ? "\nFirebase autenticado!" : "\nFalha no Firebase");
}

bool FirebaseHandler::isAuthenticated() {
    return authenticated;
}