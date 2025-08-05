#include "WebServerHandler.h"
#include <WebServer.h>

WebServerHandler::WebServerHandler(WiFiConfigurator& wifiConfig, FirebaseHandler& fbHandler) 
    : wifiConfigurator(wifiConfig), firebaseHandler(fbHandler), configured(false) {}

void WebServerHandler::begin() {
    server.on("/", HTTP_GET, [this]() { handleRoot(); });
    server.on("/configure", HTTP_POST, [this]() { handleConfigure(); });
    server.on("/firebase", HTTP_GET, [this]() { handleFirebaseConfig(); });
    server.on("/firebase-config", HTTP_POST, [this]() { handleFirebaseConfig(); });
    server.onNotFound([this]() { handleNotFound(); });
    server.begin();
}

void WebServerHandler::handleClient() {
    server.handleClient();
}

bool WebServerHandler::isConfigured() {
    return configured;
}

// Implemente os novos métodos
String WebServerHandler::getSSID() {
    return ssid;
}

String WebServerHandler::getPassword() {
    return password;
}

// Mantenha as implementações existentes para handleRoot, handleConfigure, etc...

void WebServerHandler::handleRoot() {
    // Se já estiver conectado ao Wi-Fi, redireciona para /firebase
    if(wifiConfigurator.isConnected()) {
        server.sendHeader("Location", "/firebase");
        server.send(303);
        return;
    }
    String html = R"(
        <!DOCTYPE html>
        <html>
        <head>
            <title>ESP32 WiFi Configuration</title>
            <meta name="viewport" content="width=device-width, initial-scale=1">
            <style>
                body { font-family: Arial, sans-serif; margin: 20px; }
                .container { max-width: 400px; margin: 0 auto; }
                .form-group { margin-bottom: 15px; }
                label { display: block; margin-bottom: 5px; }
                input { width: 100%; padding: 8px; box-sizing: border-box; }
                button { background-color: #4CAF50; color: white; padding: 10px 15px; border: none; cursor: pointer; }
                button:hover { background-color: #45a049; }
            </style>
        </head>
        <body>
            <div class="container">
                <h1>WiFi Configuration</h1>
                <form action="/configure" method="post">
                    <div class="form-group">
                        <label for="ssid">WiFi SSID:</label>
                        <input type="text" id="ssid" name="ssid" required>
                    </div>
                    <div class="form-group">
                        <label for="password">WiFi Password:</label>
                        <input type="password" id="password" name="password">
                    </div>
                    <button type="submit">Connect</button>
                </form>
            </div>
        </body>
        </html>
    )";
    
    server.send(200, "text/html", html);
}

void WebServerHandler::handleConfigure() {
    if (server.hasArg("ssid")) {
        ssid = server.arg("ssid");
        password = server.arg("password");
        
        if (wifiConfigurator.connectToWiFi(ssid.c_str(), password.c_str())) {
            // Salva as credenciais no Preferences
            wifiConfigurator.saveCredentials(ssid.c_str(), password.c_str());
            
            configured = true;
            wifiConfigurator.stopAP();
            server.send(200, "text/html", "<h1>Success!</h1><p>ESP32 connected to WiFi. AP will be turned off.</p>");
        } else {
            server.send(200, "text/html", "<h1>Error</h1><p>Failed to connect to WiFi. Please try again.</p>");
        }
    } else {
        server.send(400, "text/html", "<h1>Bad Request</h1><p>SSID is required</p>");
    }
}
void WebServerHandler::handleNotFound() {
    server.send(404, "text/html", "<h1>Not Found</h1>");
}
void WebServerHandler::handleFirebaseConfig() {
    if(server.method() == HTTP_GET) {
        // Mostrar formulário de login
        String html = R"(
            <!DOCTYPE html>
            <html>
            <head><title>Login Firebase</title></head>
            <body>
                <h1>Login Firebase</h1>
                <form action="/firebase-config" method="post">
                    <input type="email" name="email" placeholder="Email" required><br>
                    <input type="password" name="password" placeholder="Senha" required><br>
                    <button type="submit">Entrar</button>
                </form>
            </body>
            </html>
        )";
        server.send(200, "text/html", html);
    } else {
        String email = server.arg("email");
        String password = server.arg("password");
        
        FirebaseHandler::AuthError authResult = firebaseHandler.begin(email, password);
        
        switch(authResult) {
            case FirebaseHandler::AUTH_SUCCESS:
                server.send(200, "text/html", 
                    "<h1>Sucesso!</h1><p>Autenticado no Firebase</p>");
                break;
                
            case FirebaseHandler::AUTH_WRONG_PASSWORD:
                server.send(401, "text/html", 
                    "<h1>Erro</h1><p>Senha incorreta</p><a href='/firebase'>Tentar novamente</a>");
                break;
                
            case FirebaseHandler::AUTH_USER_NOT_FOUND:
                server.send(404, "text/html", 
                    "<h1>Erro</h1><p>Usuário não encontrado</p><a href='/firebase'>Tentar novamente</a>");
                break;
                
            case FirebaseHandler::AUTH_NETWORK_ERROR:
                server.send(503, "text/html", 
                    "<h1>Erro</h1><p>Problema de conexão</p><a href='/firebase'>Tentar novamente</a>");
                break;
                
            default:
                server.send(500, "text/html", 
                    "<h1>Erro</h1><p>" + firebaseHandler.getLastErrorMessage() + "</p><a href='/firebase'>Tentar novamente</a>");
        }
    }
}
