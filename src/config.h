#ifndef CONFIG_H
#define CONFIG_H

// ================== Версия прошивки ==================
#define FW_VERSION_MAJOR    0
#define FW_VERSION_MINOR    2
#define FW_VERSION_PATCH    0

// ================== Нагревательный узел ==================
#define HEATER_REVISION         2
#define HEATER_CHANNELS         2
#define HEATER_LAMP_COUNT       4
#define HEATER_POWER_WATTS      2000
#define HEATER_MAX_ON_TIME_S    10
#define HEATER_MIN_PAUSE_S      20

// Пины реле
#define HEATER_LEFT_PIN         19
#define HEATER_RIGHT_PIN        20

// ================== Переключатель разрешения нагрева ==================
#define HAS_HEATER_ENABLE_SWITCH 0
#if HAS_HEATER_ENABLE_SWITCH
  #define HEATER_ENABLE_PIN     10
  #define HEATER_ENABLE_GND     9
#endif

// ================== Джойстик ==================
#define HAS_JOYSTICK            0
#if HAS_JOYSTICK
  #define JOY_X_PIN             1
  #define JOY_Y_PIN             2
  #define JOY_BTN_PIN           42
  #define JOY_ADC_BITS          12
  #define JOY_ADC_MAX           ((1 << JOY_ADC_BITS) - 1)
  #define JOY_DEADZONE_MIN      15.0f
  #define JOY_NOISE_SAMPLES     200
#endif

// ================== Кнопка «Старт» (правая рукоять) ==================
#define HAS_START_BUTTON        0
#if HAS_START_BUTTON
  #define START_BTN_PIN         5
  #define START_BTN_GND         4
#endif

// ================== Общие настройки ШИМ ==================
#define LEDC_PWM_FREQ           20000   // 20 кГц — без мерцания на видео
#define LEDC_PWM_RESOLUTION     8

// ================== Индикаторный светодиод кнопки «Старт» ==================
#define HAS_START_BUTTON_LED    0
#if HAS_START_BUTTON_LED
  #define START_LED_PIN         6
#endif

// ================== Светодиодная подсветка рабочей зоны ==================
#define HAS_WORK_LIGHT          0
#if HAS_WORK_LIGHT
  #define WORK_LIGHT_CHANNELS   2
  #define WORK_LIGHT_POWER_MW   400
  #define WORK_LIGHT1_PIN       37
  #define WORK_LIGHT1_GND       36
  #define WORK_LIGHT2_PIN       3
  #define WORK_LIGHT2_GND       8
  #define WORK_LIGHT_CURRENT_MA 20
#endif

// ================== Статусный светодиод отладочной платы ==================
#define HAS_STATUS_LED          0
#if HAS_STATUS_LED
  #define STATUS_LED_PIN        38
#endif

// ================== Тайминги кнопок ==================
#define BUTTON_DEBOUNCE_MS      50
#define BTN_LONG_PRESS_MS       300

// ================== HID-мышь ==================
#define MOUSE_UPDATE_HZ         125
#define MOUSE_CURVE_EXPONENT    2.5f

#endif // CONFIG_H