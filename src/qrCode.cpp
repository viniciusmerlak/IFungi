#include "qrCode.h"

void generateQRCode(const String& data) {
    Serial.println("Gerando QR Code...");
    
    // Cria o QR Code
    QRCode qrcode;
    uint8_t qrVersion = 5;
    uint8_t qrErrorLevel = ECC_LOW;
    uint8_t buffer[qrcode_getBufferSize(qrVersion)];
    
    // Converte String para char array
    char dataChar[data.length() + 1];
    data.toCharArray(dataChar, data.length() + 1);
    
    qrcode_initText(&qrcode, buffer, qrVersion, qrErrorLevel, dataChar);

    // Imprime o QR Code no Serial
    for (uint8_t y = 0; y < qrcode.size; y++) {
        for (uint8_t x = 0; x < qrcode.size; x++) {
            Serial.print(qrcode_getModule(&qrcode, x, y) ? "██" : "  ");
        }
        Serial.println();
    }
}