#include "FirebaseHandler.h"

FirebaseHandler::FirebaseHandler() : lastErrorMessage(""), lastErrorCode(AUTH_SUCCESS) {}

FirebaseHandler::AuthError FirebaseHandler::begin(const String& email, const String& password) {
    firebaseConfig.host = FIREBASE_HOST.c_str();
    firebaseConfig.api_key = API_KEY.c_str();
    
    firebaseAuth.user.email = email.c_str();
    firebaseAuth.user.password = password.c_str();

    Firebase.reconnectWiFi(true);
    authenticated = false;
    lastErrorMessage = "";
    lastErrorCode = AUTH_SUCCESS;

    // Versão corrigida do begin()
    Firebase.begin(&firebaseConfig, &firebaseAuth); 
    
    unsigned long startTime = millis();
    while (!Firebase.ready() && (millis() - startTime < 10000)) {
        if (Firebase.isTokenExpired()) {
            lastErrorCode = AUTH_WRONG_PASSWORD;
            lastErrorMessage = "Token expirado ou credenciais inválidas";
            return lastErrorCode;
        }
        delay(100);
    }

    if (!Firebase.ready()) {
        lastErrorCode = AUTH_NETWORK_ERROR;
        lastErrorMessage = "Timeout de conexão com o Firebase";
        return lastErrorCode;
    }

    authenticated = true;
    lastErrorCode = AUTH_SUCCESS;
    return lastErrorCode;
}

bool FirebaseHandler::isAuthenticated() const {
    return authenticated && Firebase.ready();
}

String FirebaseHandler::getLastErrorMessage() const {
    return lastErrorMessage;
}

FirebaseHandler::AuthError FirebaseHandler::getLastErrorCode() const {
    return lastErrorCode;
}

void FirebaseHandler::sendData(const String& path, const String& value) {
    if(!isAuthenticated()) {
        Serial.println("Erro: Não autenticado no Firebase");
        return;
    }

    if(Firebase.setString(firebaseData, path.c_str(), value.c_str())) {
        Serial.println("Dados enviados com sucesso");
    } else {
        Serial.println("Erro ao enviar dados: " + String(firebaseData.errorReason().c_str()));
    }
}