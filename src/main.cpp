/**
 * @file main.cpp
 * @brief Главный файл прошивки дефектоскопа. Инициализация, вывод системной информации,
 *        тестовое управление каналами нагревателя через Serial.
 */

#include <Arduino.h>
#include "config.h"
#include "heater_channel.h"

HeaterChannel heaterLeft(HEATER_LEFT_PIN);
HeaterChannel heaterRight(HEATER_RIGHT_PIN);

// Чтение состояния аппаратного переключателя безопасности
bool readHeaterEnableSwitch() {
#if HAS_HEATER_ENABLE_SWITCH
    return digitalRead(HEATER_ENABLE_PIN) == LOW; // LOW = включено
#else
    return true;
#endif
}

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

    // Инициализация нагревателя
    heaterLeft.begin();
    heaterRight.begin();
#if HAS_HEATER_ENABLE_SWITCH
    pinMode(HEATER_ENABLE_PIN, INPUT_PULLUP);
#endif
    Serial.println("\nHeater channels initialized. Ready for commands.");
}

void loop() {
    bool hwEnabled = readHeaterEnableSwitch();

    heaterLeft.update();
    heaterRight.update();

    if (Serial.available()) {
        String cmd = Serial.readStringUntil('\n');
        cmd.trim();

        if (cmd == "on left") {
            if (!hwEnabled) {
                Serial.println("Hardware switch off");
            } else if (heaterLeft.isOn()) {
                Serial.println("Left already ON");
            } else if (!heaterLeft.canTurnOn()) {
                uint32_t remaining = (static_cast<uint32_t>(HEATER_MIN_PAUSE_S) * 1000U)
                                     - (millis() - heaterLeft.getTurnOffTime());
                // На ESP32 unsigned int – 32 бита, %u подходит под uint32_t
                Serial.printf("Left BLOCKED (cooldown, %u ms left)\n", remaining);
            } else {
                heaterLeft.turnOn();
                Serial.println("Left ON");
            }
        }
        else if (cmd == "off left") {
            heaterLeft.turnOff();
            Serial.println("Left OFF");
        }
        else if (cmd == "on right") {
            if (!hwEnabled) {
                Serial.println("Hardware switch off");
            } else if (heaterRight.isOn()) {
                Serial.println("Right already ON");
            } else if (!heaterRight.canTurnOn()) {
                uint32_t remaining = (static_cast<uint32_t>(HEATER_MIN_PAUSE_S) * 1000U)
                                     - (millis() - heaterRight.getTurnOffTime());
                Serial.printf("Right BLOCKED (cooldown, %u ms left)\n", remaining);
            } else {
                heaterRight.turnOn();
                Serial.println("Right ON");
            }
        }
        else if (cmd == "off right") {
            heaterRight.turnOff();
            Serial.println("Right OFF");
        }
        else if (cmd == "on both") {
            if (!hwEnabled) {
                Serial.println("Hardware switch off");
            } else {
                bool leftReady  = heaterLeft.canTurnOn() && !heaterLeft.isOn();
                bool rightReady = heaterRight.canTurnOn() && !heaterRight.isOn();

                if (leftReady && rightReady) {
                    heaterLeft.turnOn();
                    heaterRight.turnOn();
                    Serial.println("Both ON");
                } else {
                    // Выводим причины по каждому каналу
                    if (!leftReady) {
                        if (heaterLeft.isOn()) {
                            Serial.print("Left already ON");
                        } else {
                            uint32_t rem = (static_cast<uint32_t>(HEATER_MIN_PAUSE_S) * 1000U)
                                           - (millis() - heaterLeft.getTurnOffTime());
                            Serial.printf("Left BLOCKED (cooldown, %u ms left)", rem);
                        }
                        Serial.print("; ");
                    }
                    if (!rightReady) {
                        if (heaterRight.isOn()) {
                            Serial.print("Right already ON");
                        } else {
                            uint32_t rem = (static_cast<uint32_t>(HEATER_MIN_PAUSE_S) * 1000U)
                                           - (millis() - heaterRight.getTurnOffTime());
                            Serial.printf("Right BLOCKED (cooldown, %u ms left)", rem);
                        }
                    }
                    Serial.println(" -> Both NOT turned on");
                }
            }
        }
        else if (cmd == "off both") {
            heaterLeft.turnOff();
            heaterRight.turnOff();
            Serial.println("Both OFF");
        }
        else if (cmd == "status") {
            Serial.printf("HW enable: %d | Left: %d | Right: %d | LeftCan: %d | RightCan: %d\n",
                          hwEnabled,
                          heaterLeft.isOn(), heaterRight.isOn(),
                          heaterLeft.canTurnOn(), heaterRight.canTurnOn());
        }
        else if (cmd.length() > 0) {
            Serial.println("Unknown command. Use: on left/right/both, off left/right/both, status");
        }
    }

    delay(10);
}