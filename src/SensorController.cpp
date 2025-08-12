#include "SensorController.h"
#include <DHT.h>  // Para DHT22

#define DHTPIN 33      // Pino do DHT22
#define DHTTYPE DHT22 // Tipo do sensor
int bcont1 = 0;
int bcont2 = 0;
int bcont3 = 0;
int bcont4 = 0;
int bcont5 = 0;
int bcont6 = 0;
int bcont7 = 0;



DHT dht(DHTPIN, DHTTYPE);

void SensorController::begin() {
    // Inicialização do DHT22
    dht.begin();
    delay(100); // Tempo para estabilização
    
    // Verificação do DHT22
    float temp = dht.readTemperature();
    float umid = dht.readHumidity();
    
    if(isnan(temp) || isnan(umid)) {
        Serial.println("Falha ao ler DHT22 - verificando conexões");
        dhtOK = false;
    } else {
        dhtOK = true;
        Serial.println("DHT22 inicializado com sucesso");
    }

    // Inicialização do CCS811
    if(!ccs.begin()) {
        Serial.println("Falha ao iniciar CCS811 - verificando endereço I2C");
        ccsOK = false;
    } else {
        ccsOK = true;
        Serial.println("CCS811 inicializado - aguardando 20s para aquecimento");
    }


    // Inicialização do sensor MQ-7 (CO)
    while (bcont5 < 2)
    {
        Serial.println("Inicializando sensor MQ-7");
        bcont5++;
        /* code */
    }
    
    pinMode(MQ7_PIN, INPUT);
    while(bcont6<2) {
        Serial.println("MQ-7 inicializado (aquecimento necessário)");
        bcont6++;
    }



    // Inicialização do LDR (Sensor de luz)
    while (bcont7 < 2) {
        Serial.println("Inicializando sensor de luminosidade (LDR)");
        bcont7++;
    }

    pinMode(LDR_PIN, INPUT);


    // Inicialização de variáveis
    lastUpdate = 0;
    co2 = 0;
    co = 0;
    temperature = 0;
    humidity = 0;
    light = 0;
    
}

void SensorController::update() {
    // Garante leitura a cada 2 segundos
    if(millis() - lastUpdate >= 2000) {
        if(dhtOK) {
            temperature = dht.readTemperature();
            humidity = dht.readHumidity();
        }
        
        light = analogRead(LDR_PIN);
        co = analogRead(MQ7_PIN);
        
        if(ccsOK && ccs.available()) {
            co2 = ccs.geteCO2();
        }
        
        lastUpdate = millis();
    }
}

float SensorController::getTemperature() { return temperature; }
float SensorController::getHumidity() { return humidity; }
int SensorController::getCO2() { return co2; }
int SensorController::getCO() { return co; }
int SensorController::getLight() { return light; }