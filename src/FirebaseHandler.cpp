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
void FirebaseHandler::setWiFiConfigurator(WiFiConfigurator* wifiConfig) {
    this->wifiConfig = wifiConfig;
}
void FirebaseHandler::begin(const String &apiKey, const String &email, const String &password, const String &databaseUrl) {
  
    Serial.println("Begin Recebendo email:" + email);
    Serial.println("Recebendo Senha:" + password);
    config.api_key = apiKey.c_str();
    config.database_url = databaseUrl.c_str();
    
        // Limpa estado anterior
    authenticated = false;

    // Verifica tentativas máximas
    if(authAttempts >= MAX_AUTH_ATTEMPTS) {
        unsigned long waitTime = millis() - lastAuthAttempt;
        if(waitTime < AUTH_RETRY_DELAY) {
            Serial.printf("Aguarde %lu segundos antes de tentar novamente\n", 
                        (AUTH_RETRY_DELAY - waitTime)/1000);
            return;
        }
        authAttempts = 0;
    }

    authAttempts++;
    lastAuthAttempt = millis();

    Serial.println("Tentando autenticar no Firebase...");
    Serial.printf("Email: %s, Senha: %s\n", email.c_str(), password.c_str());
    
    // Configura credenciais
    auth.user.email = email.c_str();
    auth.user.password = password.c_str();
    config.token_status_callback = tokenStatusCallback; // Define o callback de status do token

    // Inicializa Firebase com autenticação
    Firebase.begin(&config, &auth);
    
    // Aguarda autenticação
    unsigned long startTime = millis();
    while (!Firebase.ready() && (millis() - startTime < 10000)) {
        Serial.print(".");
        delay(100);
    }

    // Verifica resultado
    authenticated = Firebase.ready();
    if(authenticated) {
        userUID = String(auth.token.uid.c_str());
        estufaId = "IFUNGI-" + getMacAddress();
        Serial.println("Autenticação bem-sucedida!");
        Serial.print("UID: "); Serial.println(userUID);
        authAttempts = 0;
        bool initialized = true;

        return;
    } else {
        Serial.println("Falha na autenticação: ");
        return;
    }
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
    const String api = FIREBASE_API_KEY;
    String dbUrl = DATABASE_URL;
    Serial.println("Autenticando com Firebase...");
    Serial.printf("Api: %s, dbURL: %s\n ", api.c_str(), dbUrl.c_str());
    
    // Reset completo antes de autenticar
    Firebase.reset(&config);
    Firebase.reconnectNetwork(true);
    
    // Configura credenciais

    config.api_key = api.c_str();
    auth.user.email = email.c_str();
    auth.user.password = password.c_str();
    config.database_url = dbUrl.c_str();
    config.token_status_callback = tokenStatusCallback;
    
    // Inicializa Firebase com autenticação
    Firebase.begin(&config, &auth);
    
    Serial.print("Aguardando autenticação");
    unsigned long startTime = millis();
    
    while (millis() - startTime < 20000) { // 20 segundos de timeout
        if (Firebase.ready()) {
            authenticated = true;
            userUID = String(auth.token.uid.c_str());
            estufaId = "IFUNGI-" + getMacAddress();
            
            Serial.println("\nAutenticação bem-sucedida!");
            Serial.print("UID: "); Serial.println(userUID);
            
            // Verifica se a estufa existe imediatamente após autenticar
            verificarEstufa();
            return true;
        }
        delay(500);
        Serial.print(".");
    }
    
    Serial.println("\nFalha na autenticação: Timeout");
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
void FirebaseHandler::refreshToken() {
    if (!initialized) return;
    
    Serial.println("Atualizando token Firebase...");
    Firebase.refreshToken(&config);
    lastTokenRefresh = millis();
    
    // Aguarda o token ser atualizado
    unsigned long startTime = millis();
    while (!Firebase.ready() && (millis() - startTime < 5000)) {
        delay(100);
    }
    
    if (Firebase.ready()) {
        Serial.println("Token atualizado com sucesso!");
    } else {
        Serial.println("Falha ao atualizar token!");
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
    delay(1000);
    
    Serial.println("Verificando estufa...");
    if(estufaExiste(estufaId)) {
        Serial.println("Estufa encontrada: " + estufaId);
        verificarPermissoes();
    } else {
        Serial.println("Estufa não encontrada, criando nova...");
        criarEstufaInicial(userUID, userUID);
    }
}

void FirebaseHandler::enviarDadosParaHistorico(float temp, float umid, int co2, int co, int lux, int tvocs) {
    if (!authenticated || !Firebase.ready()) {
        return;
    }

    // Usar timestamp como chave para garantir ordem cronológica
    String timestamp = String(millis());
    String path = "/historico/" + estufaId + "/" + timestamp;
    
    FirebaseJson json;
    json.set("timestamp", timestamp);
    json.set("temperatura", temp);
    json.set("umidade", umid);
    json.set("co2", co2);
    json.set("co", co);
    json.set("tvocs", tvocs); // Use the paramete
    json.set("luminosidade", lux);
    json.set("dataHora", "2023-01-01T10:00:00Z"); // Você pode preencher com tempo real se tiver RTC
    
    if (Firebase.setJSON(fbdo, path.c_str(), json)) {
        Serial.println("Dados salvos no histórico com sucesso!");
    } else {
        Serial.println("Falha ao salvar histórico: " + fbdo.errorReason());
    }
}

// Modifique o método enviarDadosSensores existente:
void FirebaseHandler::enviarDadosSensores(float temp, float umid, int co2, int co, int lux, int tvocs) {
    refreshTokenIfNeeded();
    if (!authenticated || !Firebase.ready()) {
        Serial.println("Não autenticado ou token inválido");
        return;
    }

    // Dados atuais (como antes)
    FirebaseJson json;
    json.set("sensores/temperatura", temp);
    json.set("sensores/umidade", umid);
    json.set("sensores/co2", co2);
    json.set("sensores/co", co);
    json.set("sensores/tvocs", tvocs); // Use the parameter
    json.set("sensores/luminosidade", lux);
    json.set("lastUpdate", millis());

    String path = "/estufas/" + estufaId;
    Firebase.updateNode(fbdo, path.c_str(), json);
    
    // Salvar no histórico a cada intervalo definido
    if (millis() - lastHistoricoTime > HISTORICO_INTERVAL) {
        enviarDadosParaHistorico(temp, umid, co2, co, lux, tvocs);
        lastHistoricoTime = millis();
    }
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
    
    // Verifica se o token está pronto antes de continuar
    if (!Firebase.ready()) {
        Serial.println("Token não está pronto. Aguardando...");
        unsigned long startTime = millis();
        while (!Firebase.ready() && (millis() - startTime < 10000)) {
            delay(500);
        }
        if (!Firebase.ready()) {
            Serial.println("Timeout aguardando token.");
            return;
        }
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
    sensores.set("tvocs", 0);
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
    setpoints.set("coSp", 0);
    setpoints.set("co2Sp", 0);
    setpoints.set("tvocsSp", 0);
    json.set("setpoints", setpoints);

    json.set("niveis/agua", false);

    
    String path = "/estufas/" + estufaId;
    if (Firebase.setJSON(fbdo, path.c_str(), json)) {
        Serial.println("Estufa criada com sucesso!");
        permissaoUser(userUID, estufaId);
    } else {
        Serial.print("Erro ao criar estufa: ");
        Serial.println(fbdo.errorReason());
        
        // Se falhar por token inválido, tenta renovar
        if (fbdo.errorReason().indexOf("token") >= 0) {
            Serial.println("Token inválido, tentando renovar...");
            Firebase.refreshToken(&config);
            delay(1000);
            // Tenta novamente se o token foi renovado
            if (Firebase.ready() && Firebase.setJSON(fbdo, path.c_str(), json)) {
                Serial.println("Estufa criada após renovar token!");
            }
        }
    }
}
void FirebaseHandler::enviarHeartbeat() {
    if (!authenticated || !Firebase.ready()) {
        return;
    }

    String path = getEstufasPath() + estufaId + "/status";
    
    FirebaseJson json;
    json.set("online", true);
    json.set("lastHeartbeat", millis());
    json.set("ip", WiFi.localIP().toString());
    
    if (Firebase.updateNode(fbdo, path.c_str(), json)) {
        lastHeartbeatTime = millis();
        Serial.println("Heartbeat enviado com sucesso");
    } else {
        Serial.println("Falha ao enviar heartbeat: " + fbdo.errorReason());
    }
}



unsigned long FirebaseHandler::getLastHeartbeatTime() const {
    return lastHeartbeatTime;
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
    Preferences preferences;
    if(!preferences.begin("firebase-creds", true)) {
        Serial.println("[ERRO] Falha ao abrir preferences");
        nvs_flash_erase(); // erase the NVS partition and...
        nvs_flash_init(); // initialize the NVS partition.
        return false;
    }
    
    email = preferences.getString("email", "");
    password = preferences.getString("password", "");
    preferences.end();
    
    if(email.isEmpty() || password.isEmpty()) {
        Serial.println("Nenhuma credencial encontrada");
        return false;
    }
    begin(FIREBASE_API_KEY, email, password, DATABASE_URL);
    return true;
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
            int coSp;
            int co2Sp;
            int tvocsSp;
        } setpoints;

        // Obtém todos os valores de uma vez
        bool success = true;
        success &= json->get(result, "lux"); setpoints.lux = success ? result.intValue : 0;
        success &= json->get(result, "tMax"); setpoints.tMax = success ? result.floatValue : 0;
        success &= json->get(result, "tMin"); setpoints.tMin = success ? result.floatValue : 0;
        success &= json->get(result, "uMax"); setpoints.uMax = success ? result.floatValue : 0;
        success &= json->get(result, "uMin"); setpoints.uMin = success ? result.floatValue : 0;
        success &= json->get(result, "coSp"); setpoints.coSp = success ? result.intValue : 0;
        success &= json->get(result, "co2Sp"); setpoints.co2Sp = success ? result.intValue : 0;
        success &= json->get(result, "tvocsSp"); setpoints.tvocsSp = success ? result.intValue : 0;

        if (success) {
            Serial.println("Setpoints recebidos com sucesso:");
            Serial.println("- Lux: " + String(setpoints.lux));
            Serial.println("- Temp Máx: " + String(setpoints.tMax));
            Serial.println("- Temp Mín: " + String(setpoints.tMin));
            Serial.println("- Umidade Máx: " + String(setpoints.uMax));
            Serial.println("- Umidade Mín: " + String(setpoints.uMin));
            Serial.println("- TVOCs: " + String(setpoints.tvocsSp));

            // Aplica os setpoints nos atuadores
            actuators.aplicarSetpoints(setpoints.lux, setpoints.tMin, setpoints.tMax, 
                                 setpoints.uMin, setpoints.uMax, setpoints.coSp, 
                                 setpoints.co2Sp, setpoints.tvocsSp);
        } else {
            Serial.println("Alguns setpoints não foram encontrados no JSON");
        }
    } else {
        Serial.println("Erro ao receber setpoints: " + fbdo.errorReason());
    }
}