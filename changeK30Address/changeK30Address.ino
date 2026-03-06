#include <Wire.h>

byte CheckSum(byte * buf, byte count) {
    byte sum=0;
    while (count>0) {
        sum += *buf;
        buf++;
        count--;
    }
    return sum;
}

void setup() {
    Serial.begin(9600);
    Wire.begin();
    delay(1000);

    byte changebuf[6] = {0xD0, 0x31, 0x00, 0x00, 0x69, 0x9A};
    changebuf[4] = CheckSum(changebuf, 4);
    
    Wire.beginTransmission(0x68);
    for (size_t i = 0; i<sizeof(changebuf); i++) {
        Wire.write(changebuf[i]);
    }
    Wire.endTransmission();
}

void loop() {}