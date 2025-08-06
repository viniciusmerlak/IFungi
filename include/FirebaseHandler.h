#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H
#include "perdiavontadedeviver.h"
#include <FirebaseESP32.h>

class FirebaseHandler {
public:
    void begin(const String& FIREBASE_API_KEY, const String& FIREBASE_EMAIL, const String& FIREBASE_PASSWORD, const String& DATABASE_URL);
    bool isAuthenticated();
    bool authenticated = false;
    void criarEstufaInicial(const String& usuarioCriador, const String& usuarioAtual);
    bool estufaExiste(const String& estufaId);
    void seraQeuCrio();
    bool permissaoUser(const String& userUID, const String& estufaID);
    FirebaseData fbdo;
private:
    
    FirebaseAuth auth;
    FirebaseConfig config;
    String getMacAddress(); // Função para obter o endereço MAC
    String estufaId;
    String userUID;
};

#endif