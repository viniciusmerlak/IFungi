#include "FirebaseHandler.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
#include <Preferences.h>
String FirebaseHandler::getMacAddress() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macStr[18] = {0};
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}

void FirebaseHandler::begin(const String &apiKey, const String &email, 
                          const String &password, const String &databaseUrl) {
    config.api_key = apiKey.c_str();
    auth.user.email = email.c_str();
    auth.user.password = password.c_str();
    config.database_url = databaseUrl.c_str();
    config.token_status_callback = tokenStatusCallback;

    Firebase.reconnectNetwork(true);
    Serial.println("Inicializando Firebase...");
    
    Firebase.begin(&config, &auth);
    Firebase.setDoubleDigits(2);

    int attempts = 0;
    while (!Firebase.ready() && attempts < 10) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    authenticated = Firebase.ready();
    if (authenticated) {
        userUID = String(auth.token.uid.c_str());
        estufaId = "IFUNGI-" + getMacAddress();
        Serial.println("\nAutenticado com sucesso!");
        Serial.print("UID: "); Serial.println(userUID);
    } else {
        Serial.println("\nFalha na autenticação inicial");
    }
}


void FirebaseHandler::resetAuthAttempts() {
    authAttempts = 0;
    lastAuthAttempt = 0;
    Serial.println("Contador de tentativas resetado");
}

bool FirebaseHandler::authenticate(const String& email, const String& password) {
    if (authAttempts >= MAX_AUTH_ATTEMPTS) {
        unsigned long waitTime = millis() - lastAuthAttempt;
        if (waitTime < AUTH_RETRY_DELAY) {
            Serial.printf("Aguarde %lu segundos antes de tentar novamente\n", 
                        (AUTH_RETRY_DELAY - waitTime)/1000);
            return false;
        }
        authAttempts = 0;
    }

    authAttempts++;
    lastAuthAttempt = millis();

    Serial.println("Tentando autenticar no Firebase...");
    auth.user.email = email.c_str();
    auth.user.password = password.c_str();
    Firebase.begin(&config, &auth);

    unsigned long startTime = millis();
    while (!Firebase.ready() && (millis() - startTime < 10000)) {
        delay(100);
    }

    if (Firebase.ready()) {
        authenticated = true;
        userUID = String(auth.token.uid.c_str());
        authAttempts = 0;
        Serial.println("Autenticação bem-sucedida!");
        return true;
    }

    Serial.printf("Falha na autenticação (%d/%d tentativas)\n", authAttempts, MAX_AUTH_ATTEMPTS);
    if (authAttempts >= MAX_AUTH_ATTEMPTS) {
        Serial.println("Máximo de tentativas atingido. Use /reset-auth para resetar.");
    }
    return false;
}



bool FirebaseHandler::permissaoUser(const String& userUID, const String& estufaID) {
    if (!Firebase.ready()) {
        Serial.println("Firebase não está pronto.");
        return false;
    }

    String userPath = "/Usuarios/" + userUID;
    String estufasPath = userPath + "/Estufas permitidas"; // Note o lowercase "permitidas"
    
    // Verifica se o usuário já tem permissão para esta estufa
    if (Firebase.getString(fbdo, estufasPath)) {
        if (fbdo.stringData() == estufaID) {
            Serial.println("Usuário já tem permissão para esta estufa.");
            return true;
        }
        // Se já tem uma estufa diferente, não sobrescreve (ou modifique conforme sua regra de negócio)
        Serial.println("Usuário já tem permissão para outra estufa.");
        return false;
    }

    // Se não tem permissão registrada ou ocorreu erro na verificação
    if (Firebase.setString(fbdo, estufasPath.c_str(), estufaID)) {
        Serial.println("Permissão de estufa concedida com sucesso.");
        return true;
    } else {
        Serial.print("Falha ao conceder permissão: ");
        Serial.println(fbdo.errorReason());
        
        // Se falhar, tenta criar a estrutura completa do usuário
        FirebaseJson userData;
        userData.set("Estufas permitidas", estufaID); // Note o lowercase
        
        if (Firebase.setJSON(fbdo, userPath.c_str(), userData)) {
            Serial.println("Novo usuário criado com permissão de estufa.");
            return true;
        } else {
            Serial.print("Erro crítico ao criar usuário: ");
            Serial.println(fbdo.errorReason());
            return false;
        }
    }
}

void FirebaseHandler::criarEstufaInicial(const String& usuarioCriador, const String& usuarioAtual) {
    if (!authenticated) {
        Serial.println("Usuário não autenticado.");
        return;
    }
    permissaoUser(userUID, estufaId);

    FirebaseJson json;
    FirebaseJson atuadores;
    atuadores.set("leds/ligado", false);
    atuadores.set("leds/watts", 0);
    atuadores.set("rele1", false);
    atuadores.set("rele2", false);
    atuadores.set("rele3", false);
    atuadores.set("rele4", false);
    json.set("atuadores", atuadores);

    json.set("createdBy", usuarioCriador);
    json.set("currentUser", usuarioAtual);
    json.set("lastUpdate", (int)time(nullptr));

    FirebaseJson sensores;
    sensores.set("co", 0);
    sensores.set("co2", 0);
    sensores.set("luminosidade", 0);
    sensores.set("temperatura", 0);
    sensores.set("umidade", 0);
    json.set("sensores", sensores);

    FirebaseJson setpoints;
    setpoints.set("lux", 0);
    setpoints.set("tMax", 0);
    setpoints.set("tMin", 0);
    setpoints.set("uMax", 0);
    setpoints.set("uMin", 0);
    json.set("setpoints", setpoints);

    json.set("niveis/agua", false);

    String path = "/estufas/" + estufaId;
    if (Firebase.setJSON(fbdo, path.c_str(), json)) {
        Serial.println("Estufa criada com sucesso!");
        permissaoUser(userUID, estufaId);
    } else {
        Serial.print("Erro ao criar estufa: ");
        Serial.println(fbdo.errorReason());
    }
}

bool FirebaseHandler::estufaExiste(const String& estufaId) {
    if (!authenticated) {
        Serial.println("Usuário não autenticado.");
        return false;
    }

    String path = "/estufas/" + estufaId;
    if (Firebase.get(fbdo, path.c_str())) {
        if (fbdo.dataType() != "null") {
            Serial.println("Estufa encontrada.");
            return true;
        }
    }
    
    Serial.println("Estufa não encontrada, criando nova...");
    criarEstufaInicial(userUID, userUID);
    return false;
}

void FirebaseHandler::seraQeuCrio() {
    estufaExiste(estufaId);
}

bool FirebaseHandler::isAuthenticated() const {
    return authenticated;
}

bool FirebaseHandler::loadFirebaseCredentials(String& email, String& password) {
    Preferences preferences;
    if(!preferences.begin("firebase-creds", true)) {
        Serial.println("[ERRO] Falha ao abrir preferences");
        return false;
    }
    
    email = preferences.getString("email", "");
    password = preferences.getString("password", "");
    preferences.end();
    
    if(email.isEmpty() || password.isEmpty()) {
        Serial.println("[AVISO] Credenciais do Firebase não encontradas");
        return false;
    }
    
    return true;
}