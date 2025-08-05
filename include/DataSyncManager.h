#ifndef DATA_SYNC_MANAGER_H
#define DATA_SYNC_MANAGER_H

#include <FirebaseESP32.h>
#include "SensoresManager.h"

class DataSyncManager {
public:
    DataSyncManager(FirebaseData& fbdo);  // Agora recebe referência
    void syncSensorData(const String& estufaPath, const SensoresManager::SensorReadings& data);
    void fetchSetpoints(const String& estufaPath);

private:
    FirebaseData& fbdo;  // Agora é referência
};


#endif