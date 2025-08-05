#ifndef ESTUFA_MANAGER_H
#define ESTUFA_MANAGER_H

#include <FirebaseESP32.h>

class EstufaManager {
public:
    EstufaManager(FirebaseData* fbdo);
    bool setupEstufa(const String& macAddress, const String& userUID);
    bool estufaExists(const String& estufaID);
    void createNewEstufa(const String& estufaID, const String& userUID);
    void updateUserPermissions(const String& userUID, const String& estufaID);

private:
    FirebaseData* fbdo;
    void createDefaultConfig(const String& estufaPath);
};


#endif