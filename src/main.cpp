#include <Arduino.h>
#include "config.h"

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("\n\nOK");
    delay(1000);

    Serial.println("\n=== ESP32-S3 Device Info ===");
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

    // ------------------- Аппаратная конфигурация -------------------
    Serial.println("\n=== Hardware Configuration ===");

    // Нагреватель
    Serial.println("Heater:");
    Serial.printf("  Revision: %d\n", HEATER_REVISION);
    Serial.printf("  Channels: %d\n", HEATER_CHANNELS);
    Serial.printf("  Total lamps: %d (power %d W)\n", HEATER_LAMP_COUNT, HEATER_POWER_WATTS);
    Serial.printf("  Max on time: %d s, min pause: %d s\n", HEATER_MAX_ON_TIME_S, HEATER_MIN_PAUSE_S);
    Serial.printf("  Pins: left relay=%d, right relay=%d\n", HEATER_LEFT_PIN, HEATER_RIGHT_PIN);

    // Переключатель разрешения нагрева
    Serial.printf("Heater enable switch: %s\n", HAS_HEATER_ENABLE_SWITCH ? "Present" : "Not installed");
#if HAS_HEATER_ENABLE_SWITCH
    Serial.printf("  Pin: %d (GND: %d)\n", HEATER_ENABLE_PIN, HEATER_ENABLE_GND);
#endif

    // Джойстик
    Serial.printf("Joystick: %s\n", HAS_JOYSTICK ? "Present" : "Not installed");
#if HAS_JOYSTICK
    Serial.printf("  Pins: X=%d, Y=%d, Btn=%d\n", JOY_X_PIN, JOY_Y_PIN, JOY_BTN_PIN);
    Serial.printf("  ADC bits: %d, max: %d\n", JOY_ADC_BITS, JOY_ADC_MAX);
    Serial.printf("  Dead zone: %.1f, noise samples: %d\n", JOY_DEADZONE_MIN, JOY_NOISE_SAMPLES);
#endif

    // Кнопка Старт
    Serial.printf("Start button: %s\n", HAS_START_BUTTON ? "Present" : "Not installed");
#if HAS_START_BUTTON
    Serial.printf("  Pin: %d (GND: %d)\n", START_BTN_PIN, START_BTN_GND);
#endif

    // Индикатор кнопки Старт
    Serial.printf("Start button LED: %s\n", HAS_START_BUTTON_LED ? "Present" : "Not installed");
#if HAS_START_BUTTON_LED
    Serial.printf("  Pin: %d\n", START_LED_PIN);
#endif

    // Подсветка рабочей зоны
    Serial.printf("Work light: %s\n", HAS_WORK_LIGHT ? "Present" : "Not installed");
#if HAS_WORK_LIGHT
    Serial.printf("  Channels: %d\n", WORK_LIGHT_CHANNELS);
    Serial.printf("  Total power: %d mW, max current per LED: %d mA\n",
                  WORK_LIGHT_POWER_MW, WORK_LIGHT_CURRENT_MA);
    Serial.printf("  LED1: pin=%d, GND=%d\n", WORK_LIGHT1_PIN, WORK_LIGHT1_GND);
    Serial.printf("  LED2: pin=%d, GND=%d\n", WORK_LIGHT2_PIN, WORK_LIGHT2_GND);
#endif

    // Статусный LED платы
    Serial.printf("Onboard status LED: %s\n", HAS_STATUS_LED ? "Present" : "Not installed");
#if HAS_STATUS_LED
    Serial.printf("  Pin: %d\n", STATUS_LED_PIN);
#endif

    // Общие настройки ШИМ
    Serial.println("\n=== PWM Settings ===");
    Serial.printf("Frequency: %d Hz, resolution: %d bits\n", LEDC_PWM_FREQ, LEDC_PWM_RESOLUTION);

    // Тайминги кнопок
    Serial.println("\n=== Button Timings ===");
    Serial.printf("Debounce: %d ms, long press: %d ms\n", BUTTON_DEBOUNCE_MS, BTN_LONG_PRESS_MS);

    // HID-мышь
    Serial.println("\n=== HID Mouse Settings ===");
    Serial.printf("Update rate: %d Hz, curve exponent: %.2f\n", MOUSE_UPDATE_HZ, MOUSE_CURVE_EXPONENT);

    // Версия прошивки
    Serial.println("\n=== Firmware Version ===");
    Serial.printf("%d.%d.%d\n", FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH);
}

void loop() {
    delay(1000);
}