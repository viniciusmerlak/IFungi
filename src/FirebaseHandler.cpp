#include "FirebaseHandler.h"
#include "WebServer.h"
#include <addons/TokenHelper.h>
#include <addons/RTDBHelper.h>
String FirebaseHandler::getMacAddress() {
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macStr[18] = {0};
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    return String(macStr);
}
unsigned long count = 0;
unsigned long sendDataPrevMillis = 0;

void FirebaseHandler::begin(const String &FIREBASE_API_KEY, const String &FIREBASE_EMAIL, const String &FIREBASE_PASSWORD, const String &DATABASE_URL) {
   
    


    config.api_key = FIREBASE_API_KEY.c_str();
    auth.user.email = FIREBASE_EMAIL.c_str();
    auth.user.password = FIREBASE_PASSWORD.c_str();
    config.database_url = DATABASE_URL.c_str();
    config.token_status_callback = tokenStatusCallback; // see addons/TokenHelper.h
    

   
   
    Firebase.reconnectNetwork(true);

    Serial.println("Inicializando Firebase...");
    Firebase.begin(&config, &auth);
    Firebase.setDoubleDigits(2);

    // Espera com timeout
    int attempts = 0;
    while (!Firebase.ready() && attempts < 10) {
        delay(500);
        Serial.print(".");
        if (!Firebase.ready() && MB_String(auth.token.uid).length() == 0) {
            Serial.println("\nErro de autenticação: UID não recebido.");
            authenticated = false;

            return;
        };
        attempts++;
    };

    authenticated = Firebase.ready();
        if (authenticated) {
        userUID = String(auth.token.uid.c_str());
        estufaId = "IFUNGI-" + getMacAddress(); // Inicializado aqui
    }

    Serial.print("UID do usuário: " + userUID + "\n");
    Serial.println(authenticated ? "\nFirebase autenticado!" : "\nFalha no Firebase");



}
bool FirebaseHandler::permissaoUser(const String& userUID, const String& estufaID) {
    if (!Firebase.ready()) {
        Serial.println("Firebase não está pronto.");
        return false;
    }

    String userPath = "/usuarios/" + userUID;
    
    // Verifica se o usuário existe
    if (Firebase.get(fbdo, userPath.c_str())) {
        if (fbdo.dataType() == "null") {
            Serial.println("Usuário não encontrado, criando novo registro...");
            
            // Cria o nó do usuário com a estufa permitida
            FirebaseJson userData;
            FirebaseJson estufasPermitidas;
            estufasPermitidas.add(estufaID);
            userData.set("Estufas Permitidas", estufasPermitidas);
            
            if (Firebase.setJSON(fbdo, userPath.c_str(), userData)) {
                Serial.println("Novo usuário criado com estufa permitida.");
                return true;
            }
        } else {
            // Usuário existe, atualiza as estufas permitidas
            String estufasPath = userPath + "/Estufas Permitidas";
            
            // Adiciona a nova estufa ao array existente
            if (Firebase.pushString(fbdo, estufasPath.c_str(), estufaID)) {
                Serial.println("Estufa adicionada às permissões do usuário.");
                return true;
            }
        }
    }
    
    Serial.print("Erro ao atualizar usuário: ");
    Serial.println(fbdo.errorReason());
    return false;
}
void FirebaseHandler::criarEstufaInicial(const String& usuarioCriador, const String& usuarioAtual) {
  if (!Firebase.ready()) {
    Serial.println("Firebase não está pronto.");
    return;
  }
  

  // Gera o UID da estufa (IFUNGI-MAC)
  String estufaUID = "IFUNGI-" + getMacAddress();
  permissaoUser(usuarioCriador, estufaUID); // Adiciona permissão para o usuário criador
  // Caminho no formato /estufas/[estufaUID]/
  String path = "/estufas/" + estufaUID;
  
  FirebaseJson json;

  // Estrutura dos dados
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

  Serial.printf("Criando estufa com UID: %s...\n", estufaUID.c_str());
  if (Firebase.setJSON(fbdo, path.c_str(), json)) {
    Serial.println("Estufa criada com sucesso!");
  } else {
    Serial.print("Erro ao criar estufa: ");
    Serial.println(fbdo.errorReason());
  }
}

bool FirebaseHandler::estufaExiste(const String& estufaId) {
  if (!Firebase.ready()) {
    Serial.println("Firebase não está pronto.");
    return false;
  }

  String path = "/estufas/" + estufaId;

  Serial.printf("Verificando existência da estufa %s...\n", estufaId.c_str());

  if (Firebase.get(fbdo, path.c_str())) {  // Corrigido aqui
    if (fbdo.dataType() != "null") {

      if (authenticated == true){
          Serial.println("Estufa encontrada!");
      }else{
          Serial.println("Usuário não autenticado, não é possível criar estufa.");

      }

      return true;
    } else {
      Serial.println("Estufa não encontrada (null).");
      criarEstufaInicial(userUID, userUID);
    }
  } else {
    Serial.printf("Erro ao verificar estufa: %s\n", fbdo.errorReason().c_str());
    criarEstufaInicial(userUID, userUID);
  }

  return false;
}

void FirebaseHandler::seraQeuCrio() {
  estufaExiste(estufaId);
}


bool FirebaseHandler::isAuthenticated() {
    return authenticated;
}