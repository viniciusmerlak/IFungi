// ActuatorController.cpp
#include "ActuatorController.h"
int cont1 = 0;
int cont2 = 0;
int cont3 = 0;
void ActuatorController::begin(uint8_t pinLED, uint8_t pinRele1, uint8_t pinRele2, 
                             uint8_t pinRele3, uint8_t pinRele4) {
    Serial.println("Inicializando ActuatorController...");
    _pinRele1 = pinRele1;
    _pinRele2 = pinRele2;
    _pinRele3 = pinRele3;
    _pinRele4 = pinRele4;

    pinMode(_pinLED, OUTPUT);
    pinMode(_pinRele1, OUTPUT);
    pinMode(_pinRele2, OUTPUT);
    pinMode(_pinRele3, OUTPUT);
    pinMode(_pinRele4, OUTPUT);
}
void ActuatorController::aplicarSetpoints(int lux, float tMin, float tMax, float uMin, float uMax) {
    // Armazena os setpoints para uso no controle automático
    this->luxSetpoint = lux;
    this->tempMin = tMin;
    this->tempMax = tMax;
    this->umidMin = uMin;
    this->umidMax = uMax;

    // Opcional: já aplica alguma ação imediata se necessário
    Serial.println("Novos setpoints aplicados nos atuadores");
}
void ActuatorController::controlarLEDs(bool ligado, int watts) {
    while (cont3>2){
        Serial.printf("Controlando LEDs: %s, Watts: %d\n", ligado ? "Ligado" : "Desligado", watts);
        cont3++;
    }
    
    digitalWrite(_pinLED, ligado ? HIGH : LOW);
    // Implemente controle de watts/PWM se necessário
}

void ActuatorController::controlarRele(uint8_t num, bool estado) {

    while (cont1<2) {
        cont1++;
        Serial.printf("Controlando Rele %d: %s\n", num, estado ? "Ligado" : "Desligado");
    }
    
    switch(num) {
        // Rele 1 Liga e Desliga a Peltier
        case 1: digitalWrite(_pinRele1, estado ? HIGH : LOW); break;
        //Rele 2 Escolhe entre Peltier resfriar ou aquecer
        case 2: digitalWrite(_pinRele2, estado ? HIGH : LOW); break;
        // Rele 3 Liga e Desliga o Umidificador
        case 3: digitalWrite(_pinRele3, estado ? HIGH : LOW); break;
        // Rele 4 Liga e Desliga o Exaustor
        case 4: digitalWrite(_pinRele4, estado ? HIGH : LOW); break;
    }
}

void ActuatorController::controlarPeltier(bool resfriar, int potencia) {
    if (resfriar) {
        // Modo resfriamento
        digitalWrite(_pinRele1, HIGH);
        digitalWrite(_pinRele2, LOW);
        Serial.println("Peltier em modo resfriamento");
        delay(1000); // Simula tempo de resfriamento
    } else {
        // Modo aquecimento ou desligado
        digitalWrite(_pinRele1, LOW);
        digitalWrite(_pinRele2, potencia > 0 ? HIGH : LOW);
        Serial.println(potencia > 0 ? "Peltier em modo aquecimento" : "Peltier desligado");
        delay(1000); // Simula tempo de aquecimento
    }
}

bool ActuatorController::AquecerPastilha(bool ligar) {
    if (ligar) {
        if (!peltierHeating || (millis() - lastPeltierTime > peltierTimeout)) {
            Serial.println("Iniciando aquecimento da pastilha");
            controlarPeltier(false, 100); // Aquecer em 100%
            lastPeltierTime = millis();
            peltierHeating = true;
            return true;
        }
        Serial.println("Pastilha já em aquecimento - aguardando timeout");
        delay(1000); // Aguarda antes de tentar novamente
        return false;
    } else {
        if (peltierHeating) {
            Serial.println("Desligando aquecimento da pastilha");
            controlarPeltier(false, 0); // Desligar
            peltierHeating = false;
            delay(1000); // Aguarda antes de permitir novo aquecimento
        }
        return false;
    }
}

void ActuatorController::controlarAutomaticamente(float temp, float umid, int luz) {
    Serial.printf("Controlando: Temp=%.2f, Umid=%.2f, Luz=%d\n", temp, umid, luz);
    


    // Controle de Temperatura (Peltier)
    if (temp > tempMax) {
        // Resfriar
        AquecerPastilha(false); // Desliga aquecimento se estiver ligado
        controlarPeltier(true, 100); // Liga resfriamento
    } 
    else if (temp < tempMin) {
        // Aquecer
        AquecerPastilha(true);
    } 
    else {
        // Temperatura OK
        AquecerPastilha(false);
        controlarPeltier(false, 0); // Desliga completamente
    }

    // Controle de Umidade
    if (umid < umidMin) {
        controlarRele(3, true); // Liga umidificador
    } 
    else if (umid > umidMax) {
        controlarRele(3, false); // Desliga umidificador
    }

    // Controle de Luz (exemplo)
    int luzNecessaria = max(0, luxSetpoint - luz);
    controlarLEDs(luzNecessaria > 0, luzNecessaria);
    delay(1000); // Aguarda 1 segundo antes de nova leitura
}
/*


não implementado ainda

void ActuatorController::controlarGases(int co2, int co) {
    // Implemente o controle de gases aqui
    Serial.printf("Controlando gases: CO2=%d, CO=%d\n", co2, co);
    // Exemplo: se CO2 > 1000 ppm, ligar exaustor
    if (co2 > 1000) {
        controlarRele(4,true);
    } else {
        controlarRele(4,false);
    }
    if(co > 1000) {
        Serial.println("Níveis de CO altos, tomando ação!");
        // Aqui você pode adicionar lógica para lidar com altos níveis de CO
    } else {
        Serial.println("Níveis de CO normais.");
    }
}



*/

