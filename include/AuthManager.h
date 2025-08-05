#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <FirebaseESP32.h>

class AuthManager {
public:
    struct UserAuth {
        String uid;
        String email;
        bool isAuthenticated;
    };

    AuthManager(FirebaseData* fbdo);
    bool authenticateUser(const String& email, const String& password);
    bool checkUserPermission(const String& userUID, const String& estufaID);
    UserAuth getCurrentUser() const;

private:
    FirebaseData* fbdo;
    UserAuth currentUser;
};

#endif