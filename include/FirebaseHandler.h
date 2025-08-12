#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include "perdiavontadedeviver.h"
#include <FirebaseESP32.h>
#include <nvs_flash.h>
#include "ActuatorController.h"  // Inclua o header completo aqui
// Forward declaration para evitar dependência circular
class ActuatorController;
    void controlarLEDs(bool ligado, int watts);
    void controlarRele(int num, bool estado);
class FirebaseHandler {
public:
    // Configuração e autenticação
    void begin(const String& apiKey, const String& email, const String& password, const String& databaseUrl);
    bool isAuthenticated() const;
    bool authenticate(const String& email, const String& password);
    void resetAuthAttempts();
    
    // Gerenciamento de estufa
    bool permissaoUser(const String& userUID, const String& estufaID);
    void criarEstufaInicial(const String& usuarioCriador, const String& usuarioAtual);
    bool estufaExiste(const String& estufaId);
    void verificarEstufa();
    void verificarPermissoes();
    
    // Funções de dados
    void enviarDadosSensores(float temp, float umid, int co2, int co, int lux);
    void verificarComandos(ActuatorController& actuators);
    void RecebeSetpoint(ActuatorController& actuators);
    // Funções auxiliares (mantidas da versão anterior)
    void seraQeuCrio();  // Pode ser substituída por verificarEstufa()
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
    
    // Constantes para paths do Firebase
    const String ESTUFAS_PATH = "/estufas/";
    const String USUARIOS_PATH = "/Usuarios/";
};

#endif