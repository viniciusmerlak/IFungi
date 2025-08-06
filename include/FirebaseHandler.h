#ifndef FIREBASE_HANDLER_H
#define FIREBASE_HANDLER_H

#include <FirebaseESP32.h>

class FirebaseHandler {
public:
    void begin(const String& FIREBASE_API_KEY, const String& FIREBASE_EMAIL, const String& FIREBASE_PASSWORD);
    bool isAuthenticated();
    
private:
    FirebaseData fbdo;
    FirebaseAuth auth;
    FirebaseConfig config;
    bool authenticated = false;
};

#endif