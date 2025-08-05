#ifndef SENSORES_MANAGER_H
#define SENSORES_MANAGER_H

#include <DHT.h>

class SensoresManager {
public:
    struct SensorReadings {
        float temperature;
        float humidity;
        int co2;
        int luminosity;
    };

    SensoresManager(uint8_t dhtPin, uint8_t dhtType);
    SensorReadings readSensors();

private:
    DHT dht;
    // Adicione outros sensores conforme necessário
};

#endif