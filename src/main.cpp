#include <Arduino.h>

#define MY_LED 38

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    pinMode(MY_LED, OUTPUT);
    digitalWrite(MY_LED, LOW);

    Serial.println("\n\nOK");

    delay(1000);

    Serial.println("\n=== ESP32-S3 Test ===");
    Serial.printf("Chip model: %s\n", ESP.getChipModel());
    Serial.printf("Chip revision: %d\n", ESP.getChipRevision());
    Serial.printf("CPU frequency: %d MHz\n", ESP.getCpuFreqMHz());
    Serial.printf("Flash size: %d MB\n", ESP.getFlashChipSize() / (1024 * 1024));
    Serial.printf("Free heap: %d bytes\n", ESP.getFreeHeap());
    
#ifdef BOARD_HAS_PSRAM
    Serial.printf("PSRAM size: %d bytes\n", ESP.getPsramSize());
    Serial.printf("Free PSRAM: %d bytes\n", ESP.getFreePsram());
#else
    Serial.println("PSRAM not enabled");
#endif

    Serial.println("Blink starting...");
}

void loop() {
    Serial.println("Tick");
    digitalWrite(MY_LED, HIGH);
    delay(500);
    digitalWrite(MY_LED, LOW);
    delay(500);
}