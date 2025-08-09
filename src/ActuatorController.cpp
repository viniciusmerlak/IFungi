// ActuatorController.cpp
#include "ActuatorController.h"
int cont1 = 0;
int cont2 = 0;
int cont3 = 0;
void ActuatorController::begin(uint8_t pinLED, uint8_t pinRele1, uint8_t pinRele2, 
                             uint8_t pinRele3, uint8_t pinRele4) {
    Serial.println("Inicializando ActuatorController...");
    _pinLED = pinLED; // Faltava esta linha
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

void ActuatorController::setFirebaseHandler(FirebaseHandler* handler) {
    firebaseHandler = handler;
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
    analogWrite(_pinLED, watts); // Ajusta a intensidade dos LEDs
    
    // Atualiza Firebase após mudança
    if(firebaseHandler != nullptr) {
        pinMode(_pinRele1, INPUT);
        pinMode(_pinRele2, INPUT);
        pinMode(_pinRele3, INPUT);
        pinMode(_pinRele4, INPUT);
        firebaseHandler->atualizarEstadoAtuadores(


            digitalRead(_pinRele1),
            digitalRead(_pinRele2),
            digitalRead(_pinRele3),
            digitalRead(_pinRele4),

            ligado,
            watts
        );
        pinMode(_pinRele1, OUTPUT);
        pinMode(_pinRele2, OUTPUT);
        pinMode(_pinRele3, OUTPUT);
        pinMode(_pinRele4, OUTPUT);
    }
}

void ActuatorController::controlarRele(uint8_t num, bool estado) {
    while (cont1<2) {
        cont1++;
        Serial.printf("Controlando Rele %d: %s\n", num, estado ? "Ligado" : "Desligado");
    }
    
    switch(num) {
        case 1: 
            digitalWrite(_pinRele1, estado ? HIGH : LOW); 
            break;
        case 2: 
            digitalWrite(_pinRele2, estado ? HIGH : LOW);
            break;
        case 3: 
            digitalWrite(_pinRele3, estado ? HIGH : LOW);
            break;
        case 4: 
            digitalWrite(_pinRele4, estado ? HIGH : LOW);
            break;
    }
    
    // Atualiza Firebase após mudança
    if(firebaseHandler != nullptr) {
        pinMode(_pinRele1, INPUT);
        pinMode(_pinRele2, INPUT);
        pinMode(_pinRele3, INPUT);
        pinMode(_pinRele4, INPUT);
        bool ligado = true;
        int watts = 0; // Valor padrão para watts dos LEDs
        firebaseHandler->atualizarEstadoAtuadores(


            digitalRead(_pinRele1),
            digitalRead(_pinRele2),
            digitalRead(_pinRele3),
            digitalRead(_pinRele4),

            ligado,
            watts
        );
        pinMode(_pinRele1, OUTPUT);
        pinMode(_pinRele2, OUTPUT);
        pinMode(_pinRele3, OUTPUT);
        pinMode(_pinRele4, OUTPUT);
    }
}

void ActuatorController::controlarPeltier(bool resfriar, bool potencia) {
    if (resfriar) {
        // Modo resfriamento
        digitalWrite(_pinRele1, HIGH);
        digitalWrite(_pinRele2, LOW);
        AquecerPastilha(false); // Desliga aquecimento
        Serial.println("Peltier em modo resfriamento");
        delay(1000); // Simula tempo de resfriament
    } else {
        // Modo aquecimento ou desligado
        AquecerPastilha(true); // Desliga aquecimento se estiver ligado
        Serial.println(potencia > 0 ? "Peltier em modo aquecimento" : "Peltier desligado");
        delay(1000); // Simula tempo de aquecimento
    }
    if(firebaseHandler != nullptr) {
        Serial.println("1111111111111111111111111111Atualizando estado dos atuadores no Firebase...");
        firebaseHandler->atualizarEstadoAtuadores(
            digitalRead(_pinRele1),
            digitalRead(_pinRele2),
            digitalRead(_pinRele3),
            digitalRead(_pinRele4),
            digitalRead(_pinLED),
            0  // Valor padrão para watts dos LEDs
        );
    }
    delay(1000); // Mantém o delay original
}

bool ActuatorController::AquecerPastilha(bool ligar) {
    if (ligar) {
        if (!peltierHeating || (millis() - lastPeltierTime > peltierTimeout)) {
            Serial.println("Iniciando aquecimento da pastilha");
            digitalWrite(_pinRele1, HIGH); // Liga o rele da pastilha
            digitalWrite(_pinRele2, HIGH); //Liga o rele do aquecimento
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
            digitalWrite(_pinRele1,LOW);
            digitalWrite(_pinRele2,LOW ); // Desliga o rele da pastilha e do aquecimento
            // Desligar
            peltierHeating = false;
            delay(5000); // Aguarda antes de permitir novo aquecimento
        }
        return false;
    }
}

void ActuatorController::controlarAutomaticamente(float temp, float umid, int luz) {
    Serial.printf("Controlando: Temp=%.2f, Umid=%.2f, Luz=%d\n", temp, umid, luz);
    if (tempMin>temp) {
        // Aciona aquecimento
        Serial.println("###############################Temperatura abaixo do mínimo, aquecendo##########################");
        bool resfriar = false;
        controlarPeltier(resfriar, false);
    } else if (temp > tempMax) {
        Serial.println("###############################Temperatura acima do máximo, resfriando##########################");
        // Aciona resfriamento
        bool resfriar = true;
        controlarPeltier(resfriar, false);
    } else {
        Serial.println("###############################Temperatura dentro do intervalo, desligando peltier##########################");
        // Temperatura ok, desliga peltier
        controlarPeltier(false, false);
    }

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

