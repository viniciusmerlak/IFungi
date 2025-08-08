// ActuatorController.h
#ifndef ACTUATOR_CONTROLLER_H
#define ACTUATOR_CONTROLLER_H

#include <Arduino.h>

class ActuatorController {
public:
    void begin(uint8_t pinLED, uint8_t pinRele1, uint8_t pinRele2, uint8_t pinRele3, uint8_t pinRele4);
    void controlarAutomaticamente(float temp, float umid, int luz);
    void aplicarSetpoints(int lux, float tMin, float tMax, float uMin, float uMax);
    void controlarLEDs(bool ligado, int watts);
    void controlarRele(uint8_t num, bool estado);
    void controlarPeltier(bool resfriar, int potencia);
    void controlarUmidificador(bool ligado);
    void controlarExaustor(bool ligado);
    bool AquecerPastilha(bool);
    // void controlarGases(int co2, int co);//nao implementado ainda
    unsigned long tempo = 0;
    unsigned long tempo2 = 10000;
 private:
    uint8_t _pinLED;
    uint8_t _pinRele1;
    uint8_t _pinRele2;
    uint8_t _pinRele3;
    uint8_t _pinRele4;
    float tempMin = 20.0;
    float tempMax = 25.0;
    float umidMin = 40.0;
    float umidMax = 70.0;
    int luxSetpoint = 500;
    unsigned long lastPeltierTime = 0;
    const unsigned long peltierTimeout = 10000; // 10 segundos
    bool peltierHeating = false;

    // Adicione outros pinos necessários
};

#endif