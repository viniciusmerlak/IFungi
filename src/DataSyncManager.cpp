#include "DataSyncManager.h"

DataSyncManager::DataSyncManager(FirebaseData& fbdo) : fbdo(fbdo) {}

void DataSyncManager::syncSensorData(const String& estufaPath, const SensoresManager::SensorReadings& data) {
    String path = "estufas/" + estufaPath + "/sensores/";
    
    // Usando os métodos públicos de FirebaseESP32 em vez de acessar RTDB diretamente
    Firebase.setFloat(&fbdo, path + "temperatura", data.temperature);
    Firebase.setFloat(&fbdo, path + "umidade", data.humidity);
    Firebase.setInt(&fbdo, path + "co2", data.co2);
    Firebase.setInt(&fbdo, path + "luminosidade", data.luminosity);
    Firebase.setInt(&fbdo, "estufas/" + estufaPath + "/lastUpdate", (int)time(nullptr));
}

void DataSyncManager::fetchSetpoints(const String& estufaPath) {
    // Implementação para buscar setpoints
    // Exemplo:
    // Firebase.getInt(&fbdo, "estufas/" + estufaPath + "/setpoints/tMax");
}