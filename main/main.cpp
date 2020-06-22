/**
 * Simple MSC device to OTA update esp32 S2
 * author: chegewara
 */
#include "Arduino.h"
#include "mscusb.h"
MSCusb dev;

extern bool init_disk();

void setup()
{
    Serial.begin(115200);
    if(init_disk())
        dev.begin();
}

void loop()
{
    delay(1000);
}
