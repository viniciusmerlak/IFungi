#ifndef ATUADORES_MANAGER_H
#define ATUADORES_MANAGER_H

#include <FirebaseESP32.h>
#include "SensoresManager.h"  // Inclui a definição de SensorReadings

class AtuadoresManager {
public:
    struct AtuadoresState {
        bool rele1;
        bool rele2;
        bool rele3;
        bool rele4;
    };

    AtuadoresManager(FirebaseData* fbdo);
    void controlAtuadores(const String& estufaPath, const SensoresManager::SensorReadings& sensorData);
    AtuadoresState getCurrentState() const;

private:
    FirebaseData* fbdo;
    AtuadoresState currentState;
};

#endif