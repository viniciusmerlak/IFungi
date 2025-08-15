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
    if(initialized) {
        Serial.println("Firebase já inicializado");
        return;
    }
    
    config.api_key = apiKey.c_str();
    config.database_url = databaseUrl.c_str();
    config.token_status_callback = tokenStatusCallback;
    
    Firebase.reconnectNetwork(true);
    Serial.println("Inicializando Firebase...");
    
    // Apenas inicializa, não autentica ainda
    Firebase.begin(&config, nullptr);
    Firebase.setDoubleDigits(2);
    initialized = true;
}


void FirebaseHandler::resetAuthAttempts() {
    authAttempts = 0;
    lastAuthAttempt = 0;
    Serial.println("Contador de tentativas resetado");
}
bool FirebaseHandler::authenticate(const String& email, const String& password) {
    if(email.isEmpty() || password.isEmpty()) {
        Serial.println("[ERRO] Email ou senha vazios");
        return false;
    }

    // Configuração do auth
    auth.user.email = email.c_str();
    auth.user.password = password.c_str();

    Serial.println("Reiniciando conexão Firebase...");
    Firebase.reset(&config);
    Firebase.begin(&config, &auth);

    Serial.print("Aguardando autenticação");
    unsigned long startTime = millis();
    
    while(millis() - startTime < 20000) {
        if(Firebase.ready()) {
            authenticated = true;
            userUID = auth.token.uid.c_str();
            estufaId = "IFUNGI-" + getMacAddress();
            
            Serial.println("\nAutenticação bem-sucedida!");
            Serial.printf("UID: %s\n", userUID.c_str());
            return true;
        }
        
        // Verifica erros específicos
        if(Firebase.getLastError() != 0) {
            Serial.printf("\nErro Firebase: %s\n", Firebase.errorToString(Firebase.getLastError()));
            break;
        }
        
        delay(500);
        Serial.print(".");
    }

    Serial.println("\n[ERRO] Falha na autenticação");
    Serial.println("Motivo: " + String(Firebase.errorToString(Firebase.getLastError())));
    return false;
}

void FirebaseHandler::atualizarEstadoAtuadores(bool rele1, bool rele2, bool rele3, bool rele4, 
                                             bool ledsLigado, int ledsWatts) {
    if (!authenticated || !Firebase.ready()) {
        Serial.println("Não autenticado para atualizar atuadores");
        return;
    }

    FirebaseJson json;
    FirebaseJson atuadores;
    
    // Adiciona estados dos relés
    atuadores.set("rele1", rele1);
    atuadores.set("rele2", rele2);
    atuadores.set("rele3", rele3);
    atuadores.set("rele4", rele4);
    
    // Adiciona estado dos LEDs
    FirebaseJson leds;
    leds.set("ligado", ledsLigado);
    leds.set("watts", ledsWatts);
    atuadores.set("leds", leds);
    
    // Adiciona timestamp
    json.set("lastUpdate", millis());
    json.set("atuadores", atuadores);

    String path = FirebaseHandler::getEstufasPath() + estufaId;
    
    if (Firebase.updateNode(fbdo, path.c_str(), json)) {
        Serial.println("Estados dos atuadores atualizados com sucesso!");
    } else {
        Serial.println("Falha ao atualizar atuadores: " + fbdo.errorReason());
    }
}

void FirebaseHandler::refreshTokenIfNeeded() {
    if(!initialized || !authenticated) return;
    
    if(millis() - lastTokenRefresh > TOKEN_REFRESH_INTERVAL) {
        Serial.println("Atualizando token Firebase...");
        Firebase.refreshToken(&config);
        lastTokenRefresh = millis();
    }
}

void FirebaseHandler::verificarEstufa() {
    if(!authenticated) {
        Serial.println("Usuário não autenticado. Verifique as credenciais.");
        return;
    }
    
    Serial.println("Verificando estufa...");
    if(estufaExiste(estufaId)) {
        Serial.println("Estufa encontrada: " + estufaId);
        verificarPermissoes();
    } else {
        Serial.println("Estufa não encontrada, criando nova...");
        criarEstufaInicial(userUID, userUID);
    }
}
void FirebaseHandler::enviarDadosSensores(float temp, float umid, int co2, int co, int lux) {
    refreshTokenIfNeeded(); // Verifica o token antes de cada operação
    if (!authenticated || !Firebase.ready()) {
       Serial.println("Não autenticado ou token inválido");
        return;
    }

    FirebaseJson json;
    json.set("sensores/temperatura", temp);
    json.set("sensores/umidade", umid);
    json.set("sensores/co2", co2);
    json.set("sensores/co", co);
    json.set("sensores/luminosidade", lux);
    json.set("lastUpdate", millis());

    String path = "/estufas/" + estufaId;
    Firebase.updateNode(fbdo, path.c_str(), json);
}

void FirebaseHandler::handleTokenError() {
    if (!Firebase.ready()) {
        Serial.println("Token inválido ou expirado. Tentando reautenticar...");
        authenticated = false;
        if (authenticate(auth.user.email.c_str(), auth.user.password.c_str())) {
            Serial.println("Reautenticação bem-sucedida.");
            verificarEstufa();
        } else {
            Serial.println("Falha na reautenticação.");
        }
    }
}

void FirebaseHandler::verificarComandos(ActuatorController& actuators) {
    if (!authenticated) return;

    String path = FirebaseHandler::getEstufasPath() + estufaId + "/atuadores"; // Modifique esta linha
    if (Firebase.getJSON(fbdo, path.c_str())) {
        FirebaseJson *json = fbdo.jsonObjectPtr();
        FirebaseJsonData result;
        
        // LEDs
        bool ledsLigado = false;
        int ledsWatts = 0;
        if(json->get(result, "leds/ligado")) {
            ledsLigado = result.boolValue;
            if(json->get(result, "leds/watts")) {
                ledsWatts = result.intValue;
            }
            actuators.controlarLEDs(ledsLigado, ledsWatts);
        }
        
        // Relés
        for(int i = 1; i <= 4; i++) {
            String relePath = "rele" + String(i);
            if(json->get(result, relePath.c_str())) {
                actuators.controlarRele(i, result.boolValue);
            }
        }
    }
}
void FirebaseHandler::verificarPermissoes() {
    String path = "/estufas/" + estufaId + "/currentUser";
    if (Firebase.getString(fbdo, path.c_str())) {
        if (fbdo.stringData() != userUID) {
            Serial.println("Usuário não tem permissão para esta estufa!");
            authenticated = false;
        }
    }
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
    if (!initialized) {
        Serial.println("Firebase não inicializado.");

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
    return authenticated; // Remover a declaração local que sobrescrevia
}

bool FirebaseHandler::loadFirebaseCredentials(String& email, String& password) {
    // Tenta abrir o namespace em modo leitura
    if (!preferences.begin("firebase-creds", true)) {
        Serial.println("[AVISO] Namespace 'firebase-creds' não encontrado - será criado quando necessário");
        return false;
    }

    // Verifica se as chaves existem antes de ler
    bool credenciaisExistem = preferences.isKey("email") && preferences.isKey("password");
    
    if (credenciaisExistem) {
        email = preferences.getString("email", "");
        password = preferences.getString("password", "");
        preferences.end();
        
        if (email.isEmpty() || password.isEmpty()) {
            Serial.println("[AVISO] Credenciais vazias na NVS");
            return false;
        }
        return true;
    } else {
        Serial.println("[AVISO] Credenciais do Firebase não encontradas na NVS");
        preferences.end();
        return false;
    }
}

void FirebaseHandler::RecebeSetpoint(ActuatorController& actuators) {
    if(!authenticated) {
        Serial.println("Usuário não autenticado. Não é possível receber setpoints.");
        return;
    }

    String path = "/estufas/" + estufaId + "/setpoints";
    
    if (Firebase.getJSON(fbdo, path.c_str())) {
        FirebaseJson *json = fbdo.jsonObjectPtr();
        FirebaseJsonData result;
        
        // Estrutura para armazenar os setpoints
        struct Setpoints {
            int lux;
            float tMax;
            float tMin;
            float uMax;
            float uMin;
        } setpoints;

        // Obtém todos os valores de uma vez
        bool success = true;
        success &= json->get(result, "lux"); setpoints.lux = success ? result.intValue : 0;
        success &= json->get(result, "tMax"); setpoints.tMax = success ? result.floatValue : 0;
        success &= json->get(result, "tMin"); setpoints.tMin = success ? result.floatValue : 0;
        success &= json->get(result, "uMax"); setpoints.uMax = success ? result.floatValue : 0;
        success &= json->get(result, "uMin"); setpoints.uMin = success ? result.floatValue : 0;

        if (success) {
            Serial.println("Setpoints recebidos com sucesso:");
            Serial.println("- Lux: " + String(setpoints.lux));
            Serial.println("- Temp Máx: " + String(setpoints.tMax));
            Serial.println("- Temp Mín: " + String(setpoints.tMin));
            Serial.println("- Umidade Máx: " + String(setpoints.uMax));
            Serial.println("- Umidade Mín: " + String(setpoints.uMin));

            // Aplica os setpoints nos atuadores
            actuators.aplicarSetpoints(setpoints.lux, setpoints.tMin, setpoints.tMax, 
                                     setpoints.uMin, setpoints.uMax);
        } else {
            Serial.println("Alguns setpoints não foram encontrados no JSON");
        }
    } else {
        Serial.println("Erro ao receber setpoints: " + fbdo.errorReason());
    }
}