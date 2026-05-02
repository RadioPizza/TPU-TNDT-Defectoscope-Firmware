#pragma once

// Пины реле ламп
#define LAMP_LEFT       19
#define LAMP_RIGHT      20

// Пины переключателя разрешения нагрева
#define LAMP_SAFETY     10
#define LAMP_SAFETY_GND 9

// Пины джойстика
#define JOY_X_PIN       1   // ADC1_CH0
#define JOY_Y_PIN       2   // ADC1_CH1
#define JOY_BTN_PIN     42

// Пины LED-подсветки рабочей области
#define LED_WORK_1      37
#define LED_WORK_1_GND  36
#define LED_WORK_2      3
#define LED_WORK_2_GND  8

// Кнопка с подсветкой на правой рукоятке
#define BTN_START_GND   4
#define BTN_START       5
#define BTN_START_LED   6

// Параметры АЦП джойстика
#define ADC_BITS        12
#define ADC_MAX         ((1 << ADC_BITS) - 1)
#define DEADZONE_MIN    15.0f
#define NOISE_SAMPLES   200

// Настройки HID мыши
#define MOUSE_UPDATE_HZ     125
#define CURVE_EXPONENT      2.5f
#define BTN_LONG_PRESS_MS   300