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
    // Limpa estado anterior
    authenticated = false;

    // Verificação básica de input
    if(email.isEmpty() || password.isEmpty() || email.indexOf('@') == -1) {
        Serial.println("[ERRO] Credenciais inválidas fornecidas");
        return false;
    }

    Serial.println("[INFO] Tentando autenticar no Firebase...");
    
    // Configura credenciais
    auth.user.email = email.c_str();
    auth.user.password = password.c_str();
    
    // Inicializa Firebase com autenticação
    Firebase.begin(&config, &auth);
    
    // Aguarda autenticação com timeout
    unsigned long startTime = millis();
    while(!Firebase.ready() && (millis() - startTime < 15000)) { // 15s timeout
        delay(250);
        Serial.print(".");
    }

    // Verifica resultado
    authenticated = Firebase.ready();
    if(authenticated) {
        userUID = String(auth.token.uid.c_str());
        estufaId = "IFUNGI-" + getMacAddress();
        Serial.println("\n[SUCESSO] Autenticação Firebase OK");
        Serial.print("[INFO] UID: "); Serial.println(userUID);
        
        // Salva credenciais apenas após autenticação bem-sucedida
        saveFirebaseCredentials(email, password);
        return true;
    } 
    
    Serial.println("\n[ERRO] Falha na autenticação");
    if(fbdo.errorReason().length() > 0) {
        Serial.print("[ERRO] Motivo: ");
        Serial.println(fbdo.errorReason());
    }
    return false;
}

void FirebaseHandler::atualizarEstadoAtuadores(bool rele1, bool rele2, bool rele3, bool rele4, 
                                             bool ledsLigado, int ledsWatts) {
    // Verificação robusta de autenticação
    if (!isAuthenticated() || !Firebase.ready()) {
        static unsigned long lastWarning = 0;
        if(millis() - lastWarning > 60000) { // Avisa a cada 1 minuto
            Serial.println("[AVISO] Não autenticado para atualizar atuadores");
            lastWarning = millis();
        }
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

    String path = getEstufasPath() + estufaId;
    
    if (Firebase.updateNode(fbdo, path.c_str(), json)) {
        Serial.println("[INFO] Estados dos atuadores atualizados com sucesso!");
    } else {
        Serial.println("[ERRO] Falha ao atualizar atuadores: " + fbdo.errorReason());
        // Tenta reautenticar em caso de falha
        if(fbdo.errorReason().indexOf("token") != -1) {
            authenticated = false;
        }
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
    if(!authenticated) {
        Serial.println("Usuário não autenticado. Dados não enviados.");
        return;
    }
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

// Adicione esta implementação:
void FirebaseHandler::saveFirebaseCredentials(const String& email, const String& password) {
    Preferences preferences;
    
    // Abre em modo leitura/escrita (cria se não existir)
    if(!preferences.begin("firebase-creds", false)) {
        Serial.println("[NVS ERRO] Falha ao acessar namespace");
        return;
    }
    
    // Verifica se os valores são válidos antes de salvar
    if(email.length() == 0 || email.indexOf('@') == -1 || password.length() == 0) {
        Serial.println("[NVS ERRO] Tentativa de salvar credenciais inválidas");
        preferences.end();
        return;
    }
    
    // Salva as credenciais
    preferences.putString("email", email);
    preferences.putString("password", password);
    
    // Confirmação imediata
    if(preferences.getString("email", "") != email || 
       preferences.getString("password", "") != password) {
        Serial.println("[NVS ERRO] Falha na verificação das credenciais salvas");
        preferences.clear();
    } else {
        Serial.println("[NVS] Credenciais salvas com sucesso");
    }
    
    preferences.end();
}
bool FirebaseHandler::loadFirebaseCredentials(String& email, String& password) {
    Preferences preferences;
    
    // Primeiro verifica se o namespace existe
    if(!preferences.begin("firebase-creds", true)) {
        Serial.println("[NVS] Namespace 'firebase-creds' não existe ainda");
        return false;
    }
    
    // Verifica se as chaves existem antes de tentar ler
    if(!preferences.isKey("email") || !preferences.isKey("password")) {
        Serial.println("[NVS] Credenciais não encontradas (primeira execução)");
        preferences.end();
        return false;
    }
    
    // Se chegou aqui, as chaves existem - pode ler com segurança
    email = preferences.getString("email", "");
    password = preferences.getString("password", "");
    preferences.end();
    
    if(email.isEmpty() || password.isEmpty()) {
        Serial.println("[NVS] Credenciais vazias");
        return false;
    }
    
    return true;
}


bool FirebaseHandler::initPreferencesNamespace() {
    Preferences preferences;
    
    // Tentativa de abrir em modo leitura/escrita (cria se não existir)
    if(!preferences.begin("firebase-creds", false)) {
        Serial.println("[ERRO CRÍTICO] Falha ao criar/abrir namespace 'firebase-creds'");
        return false;
    }
    
    // Verificação robusta para determinar se o namespace está vazio
    bool namespaceVazio = true;
    
    // Verifica algumas chaves conhecidas
    if(preferences.isKey("email") || preferences.isKey("password")) {
        namespaceVazio = false;
    }
    // Verificação adicional para outras chaves que você possa usar
    else if(preferences.isKey("wifi_ssid") || preferences.isKey("wifi_pass")) {
        namespaceVazio = false;
    }
    
    if(namespaceVazio) {
        Serial.println("[NVS] Namespace criado (vazio) - primeira execução?");
        
        // Opcional: cria entradas padrão se necessário
        preferences.putString("init_flag", "initialized");
    } else {
        Serial.println("[NVS] Namespace aberto (contém dados)");
    }
    
    preferences.end();
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