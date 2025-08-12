#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include "perdiavontadedeviver.h"
#include <FirebaseESP32.h>
#include "ActuatorController.h"

class ActuatorController;

class FirebaseHandler {
public:
    // Configuração e autenticação
    void begin(const String& apiKey, const String& email, const String& password, const String& databaseUrl);
    bool isAuthenticated() const;
    bool authenticate(const String& email, const String& password);
    void resetAuthAttempts();
    void atualizarEstadoAtuadores(bool rele1, bool rele2, bool rele3, bool rele4, bool ledsLigado, int ledsWatts);
    // No FirebaseHandler.h, adicione na seção pública:
    void saveFirebaseCredentials(const String& email, const String& password);      
    // Gerenciamento de estufa
    bool permissaoUser(const String& userUID, const String& estufaID);
    void criarEstufaInicial(const String& usuarioCriador, const String& usuarioAtual);
    bool estufaExiste(const String& estufaId);
    void verificarEstufa();
    void verificarPermissoes();
    
    // Tratamento de erros
    void handleTokenError();
    
    // Funções de dados
    void enviarDadosSensores(float temp, float umid, int co2, int co, int lux);
    void verificarComandos(ActuatorController& actuators);
    void RecebeSetpoint(ActuatorController& actuators);
    bool initPreferencesNamespace();
    // Funções auxiliares
    void seraQeuCrio();
    bool loadFirebaseCredentials(String& email, String& password);
    void refreshTokenIfNeeded();
    
    // Variáveis públicas
    FirebaseData fbdo;
    String estufaId;
    String userUID;
    bool authenticated = false;
    
    // Métodos para obter paths
    static String getEstufasPath() { return "/estufas/"; }
    static String getUsuariosPath() { return "/Usuarios/"; }

private:
    // Métodos privados
    String getMacAddress();
    
    // Configurações do Firebase
    FirebaseAuth auth;
    FirebaseConfig config;
    
    // Credenciais
    const String FIREBASE_API_KEY = "AIzaSyDkPzzLHykaH16FsJpZYwaNkdTuOOmfnGE";
    const String DATABASE_URL = "pfi-ifungi-default-rtdb.firebaseio.com";
    
    // Controle de estado
    bool initialized = false;
    
    // Controle de autenticação
    unsigned long lastAuthAttempt = 0;
    int authAttempts = 0;
    static const int MAX_AUTH_ATTEMPTS = 3;
    static const unsigned long AUTH_RETRY_DELAY = 300000; // 5 minutos
    
    // Controle de token
    static const int TOKEN_REFRESH_INTERVAL = 30 * 60 * 1000; // 30 minutos
    unsigned long lastTokenRefresh = 0;
};

#endif