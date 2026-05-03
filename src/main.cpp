#include <Arduino.h>
#include "config.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

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

    Serial.println("\n=== Heater Properties ===");
    Serial.printf("Heater revision: %d\n", HEATER_REVISION);
    Serial.printf("Lamp count: %d\n", HEATER_LAMP_COUNT);
    Serial.printf("Total power: %d W\n", HEATER_POWER_WATTS);
    Serial.printf("Max continuous on time: %d s\n", HEATER_MAX_ON_TIME_S);
    Serial.printf("Min pause between pulses: %d s\n", HEATER_MIN_PAUSE_S);

    Serial.println("\n=== Firmware Version ===");
    Serial.printf("%d.%d.%d\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);
}

void loop() {
    // Ничего не делаем — только вывод конфигурации при старте
    delay(1000);
}