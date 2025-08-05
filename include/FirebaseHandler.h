#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <FirebaseESP32.h>
#include <WiFi.h>

class FirebaseHandler {
public:
    enum AuthError {
        AUTH_SUCCESS,
        AUTH_WRONG_PASSWORD,
        AUTH_USER_NOT_FOUND,
        AUTH_NETWORK_ERROR,
        AUTH_UNKNOWN_ERROR
    };

    FirebaseHandler();
    AuthError begin(const String& email, const String& password);
    bool isAuthenticated() const;
    String getLastErrorMessage() const;
    AuthError getLastErrorCode() const;  // Adicionado
    void sendData(const String& path, const String& value);
    
private:
    const String FIREBASE_HOST = "pfi-ifungi-default-rtdb.firebaseio.com";
    const String FIREBASE_AUTH = "wmfRxP5Ex4perkccxS7zs3J5ivDqp8WIiSwZOGa0";
    const String API_KEY = "AIzaSyDkPzzLHykaH16FsJpZYwaNkdTuOOmfnGE";


    FirebaseData firebaseData;
    FirebaseAuth firebaseAuth;
    FirebaseConfig firebaseConfig;
    bool authenticated = false;
    String lastErrorMessage;
    AuthError lastErrorCode = AUTH_SUCCESS;  // Adicionado
};

#endif