#include "WebServerHandler.h"

WebServerHandler::WebServerHandler(WiFiConfigurator& wifiConfig, FirebaseHandler& fbHandler) 
    : wifiConfigurator(wifiConfig), firebaseHandler(fbHandler) {}

void WebServerHandler::saveFirebaseCredentials(const String& email, const String& password) {
    Preferences preferences;
    if(!preferences.begin("firebase-creds", false)) {
        Serial.println("[ERRO] Falha ao abrir namespace das Preferences");
        return;
    }
    
    if(!preferences.putString("email", email)) {
        Serial.println("[ERRO] Falha ao salvar email");
    }
    if(!preferences.putString("password", password)) {
        Serial.println("[ERRO] Falha ao salvar senha");
    }
    
    preferences.end();
    Serial.println("Credenciais do Firebase salvas com sucesso");
}
bool WebServerHandler::loadFirebaseCredentials(String& email, String& password) {
    Preferences preferences;
    
    // Tenta abrir o namespace em modo leitura
    if(!preferences.begin("firebase-creds", true)) {
        Serial.println("[AVISO] Namespace 'firebase-creds' não encontrado. Criando novo...");
        
        // Tenta criar o namespace se não existir
        if(!preferences.begin("firebase-creds", false)) {
            Serial.println("[ERRO CRÍTICO] Falha ao criar namespace das Preferences");
            return false;
        }
        preferences.end();
        return false;
    }
    
    email = preferences.getString("email", "");
    password = preferences.getString("password", "");
    preferences.end();
    
    bool credentialsValid = !email.isEmpty() && !password.isEmpty();
    if(!credentialsValid) {
        Serial.println("[AVISO] Credenciais não encontradas ou inválidas");
    }
    
    return credentialsValid;
}
void WebServerHandler::begin(bool wifiConnected) {
    this->wifiConnected = wifiConnected;
    if(wifiConnected) {
        String email, password;
        if(loadFirebaseCredentials(email, password)) {
            Serial.println("Credenciais do Firebase carregadas da memória");
            firebaseHandler.begin(FIREBASE_API_KEY, email, password, DATABASE_URL);
        } else {
            Serial.println("Nenhuma credencial do Firebase encontrada na memória");
        }
    }
    if(wifiConnected) {
        server.on("/", HTTP_GET, [this]() { handleFirebaseConfig(); });
    } else {
        server.on("/", HTTP_GET, [this]() { handleWiFiConfig(); });
    }
    
    server.on("/wifi-config", HTTP_POST, [this]() { handleWiFiConfig(); });
    server.on("/firebase-config", HTTP_POST, [this]() { handleFirebaseConfig(); });
    server.onNotFound([this]() { handleNotFound(); });
    server.begin();
}


void WebServerHandler::handleClient() {
    server.handleClient();
}

void WebServerHandler::handleRoot() {
    if(wifiConnected) {
        handleFirebaseConfig();
    } else {
        handleWiFiConfig();
    }
}

void WebServerHandler::handleWiFiConfig() {
    if(server.method() == HTTP_POST) {
        String ssid = server.arg("ssid");
        String password = server.arg("password");
        
        if(wifiConfigurator.connectToWiFi(ssid.c_str(), password.c_str())) {
            wifiConfigurator.saveCredentials(ssid.c_str(), password.c_str());
            server.send(200, "text/html", "<h1>WiFi conectado!</h1><p>Reiniciando para acessar modo normal...</p>");
            delay(1000);
            ESP.restart();
        } else {
            server.send(200, "text/html", "<h1>Falha na conexão</h1><a href='/'>Tentar novamente</a>");
        }
    } else {
        String html = R"(
            <!DOCTYPE html>
            <html>
            <head><title>Configurar WiFi</title></head>
            <body>
                <h1>Configurar WiFi</h1>
                <form action="/wifi-config" method="post">
                    <input type="text" name="ssid" placeholder="SSID" required><br>
                    <input type="password" name="password" placeholder="Senha"><br>
                    <button type="submit">Conectar</button>
                </form>
            </body>
            </html>
        )";
        server.send(200, "text/html", html);
    }
}

void WebServerHandler::handleFirebaseConfig() {
    if(!wifiConnected) {
        server.sendHeader("Location", "/");
        server.send(302);
        return;
    }

    if(server.method() == HTTP_POST) {
        String FIREBASE_EMAIL = server.arg("email");
        String FIREBASE_PASSWORD = server.arg("password");
        Serial.println("Tentando autenticar no Firebase...");
        Serial.print("Email: "); Serial.println(FIREBASE_EMAIL);
        
        firebaseHandler.begin(FIREBASE_API_KEY, FIREBASE_EMAIL, FIREBASE_PASSWORD, DATABASE_URL);
        
        if(firebaseHandler.isAuthenticated()) {
            // Salva as credenciais apenas se a autenticação for bem-sucedida
            saveFirebaseCredentials(FIREBASE_EMAIL, FIREBASE_PASSWORD);
            server.send(200, "text/html", "<h1>Sucesso!</h1><p>Autenticado no Firebase</p>");
        } else {
            server.send(200, "text/html", "<h1>Erro</h1><p>Falha na autenticação</p><a href='/'>Tentar novamente</a>");
        }
    } else {
        // Mostra o formulário de configuração do Firebase
        String html = R"(
            <!DOCTYPE html>
            <html>
            <head><title>Configurar Firebase</title></head>
            <body>
                <h1>Configurar Firebase</h1>
                <form action="/firebase-config" method="post">
                    <input type="email" name="email" placeholder="Email" required><br>
                    <input type="password" name="password" placeholder="Senha" required><br>
                    <button type="submit">Autenticar</button>
                </form>
            </body>
            </html>
        )";
        server.send(200, "text/html", html);
    }
}

void WebServerHandler::handleNotFound() {
    server.send(404, "text/plain", "Página não encontrada");
}