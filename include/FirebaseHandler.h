#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include "perdiavontadedeviver.h"
#include <FirebaseESP32.h>

class FirebaseHandler {
public:
    void begin(const String& apiKey, const String& email, const String& password, const String& databaseUrl);
    bool isAuthenticated() const;
    bool authenticate(const String& email, const String& password);
    void resetAuthAttempts();
    bool permissaoUser(const String& userUID, const String& estufaID);
    void criarEstufaInicial(const String& usuarioCriador, const String& usuarioAtual);
    bool estufaExiste(const String& estufaId);
    void seraQeuCrio();
    bool loadFirebaseCredentials(String& email, String& password);
    
    // Variáveis públicas
    FirebaseData fbdo;
    String estufaId;
    String userUID;
    bool authenticated = false;

private:
    String getMacAddress();
    FirebaseAuth auth;
    FirebaseConfig config;
    
    // Controle de tentativas
    unsigned long lastAuthAttempt = 0;
    int authAttempts = 0;
    const int MAX_AUTH_ATTEMPTS = 3;
    const unsigned long AUTH_RETRY_DELAY = 300000; // 5 minutos
};

#endif