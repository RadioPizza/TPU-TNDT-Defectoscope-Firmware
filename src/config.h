#ifndef CONFIG_H
#define CONFIG_H

/** @defgroup version Версия прошивки
 *  @brief Семантическое версионирование прошивки дефектоскопа
 *  @{
 */
#define FW_VERSION_MAJOR    0   /**< Мажорная версия */
#define FW_VERSION_MINOR    2   /**< Минорная версия */
#define FW_VERSION_PATCH    1   /**< Патч-версия */
/** @} */

/** @defgroup heater Нагревательный узел
 *  @brief Параметры нагревательных ламп и реле
 *  @details Значения могут меняться в зависимости от ревизии аппаратной части.
 *  @{
 */
/** Версия сборки аппаратной части дефектоскопа (конструкции).
 *  - 0 — прошивка для mock-макета (нет реального железа)
 *  - 1 — первые ранние версии
 *  - 2 — версия 2026Q2 (актуальная)
 */
#define HEATER_REVISION         2

#define HEATER_CHANNELS         2   /**< Количество независимых каналов нагрева */
#define HEATER_LAMP_COUNT       4   /**< Общее количество ламп */
#define HEATER_POWER_WATTS      2000 /**< Суммарная мощность нагревателя, Вт */
#define HEATER_MAX_ON_TIME_S    10  /**< Максимальное время непрерывного включения, с */
#define HEATER_MIN_PAUSE_S      20  /**< Минимальная пауза между включениями, с */

/** Пины реле */
#define HEATER_LEFT_PIN         19  /**< Левое реле */
#define HEATER_RIGHT_PIN        20  /**< Правое реле */
/** @} */

/** @defgroup heater_enable Переключатель разрешения нагрева
 *  @brief Аппаратный ключ безопасности
 *  @{
 */
#define HAS_HEATER_ENABLE_SWITCH 0  /**< 0 — переключатель отсутствует, 1 — присутствует */
#if HAS_HEATER_ENABLE_SWITCH
  #define HEATER_ENABLE_PIN     10  /**< Сигнальный пин */
  #define HEATER_ENABLE_GND     9   /**< Пин земли */
#endif
/** @} */

/** @defgroup joystick Джойстик
 *  @brief Параметры K-Silver JH16 с датчиками Холла
 *  @{
 */
#define HAS_JOYSTICK            0   /**< 0 — джойстик не используется, 1 — активен */
#if HAS_JOYSTICK
  #define JOY_X_PIN             1   /**< Ось X (АЦП) */
  #define JOY_Y_PIN             2   /**< Ось Y (АЦП) */
  #define JOY_BTN_PIN           42  /**< Кнопка (вход с подтяжкой) */
  #define JOY_ADC_BITS          12  /**< Разрядность АЦП */
  #define JOY_ADC_MAX           ((1 << JOY_ADC_BITS) - 1) /**< Максимальное значение АЦП */
  #define JOY_DEADZONE_MIN      15.0f /**< Минимальная мёртвая зона, % */
  #define JOY_NOISE_SAMPLES     200  /**< Количество сэмплов для усреднения шума */
#endif
/** @} */

/** @defgroup start_button Кнопка «Старт» (правая рукоять)
 *  @{
 */
#define HAS_START_BUTTON        0   /**< 0 — кнопка отключена, 1 — активна */
#if HAS_START_BUTTON
  #define START_BTN_PIN         5   /**< Сигнальный пин кнопки */
  #define START_BTN_GND         4   /**< Пин земли кнопки */
#endif
/** @} */

/** @defgroup pwm Общие настройки ШИМ
 *  @{
 */
#define LEDC_PWM_FREQ           20000   /**< Частота ШИМ, Гц (20 кГц — без мерцания на видео) */
#define LEDC_PWM_RESOLUTION     8      /**< Разрешение ШИМ, бит */
/** @} */

/** @defgroup start_led Индикаторный светодиод кнопки «Старт»
 *  @{
 */
#define HAS_START_BUTTON_LED    0   /**< 0 — светодиод не используется, 1 — активен */
#if HAS_START_BUTTON_LED
  #define START_LED_PIN         6   /**< Пин управления светодиодом */
#endif
/** @} */

/** @defgroup work_light Светодиодная подсветка рабочей зоны
 *  @brief Два белых светодиода по 200 мВт
 *  @{
 */
#define HAS_WORK_LIGHT          0   /**< 0 — подсветка выключена, 1 — активна */
#if HAS_WORK_LIGHT
  #define WORK_LIGHT_CHANNELS   2   /**< Количество независимых каналов подсветки */
  #define WORK_LIGHT_POWER_MW   400  /**< Суммарная мощность подсветки, мВт */
  #define WORK_LIGHT1_PIN       37  /**< Сигнальный пин светодиода 1 */
  #define WORK_LIGHT1_GND       36  /**< Земля светодиода 1 */
  #define WORK_LIGHT2_PIN       3   /**< Сигнальный пин светодиода 2 */
  #define WORK_LIGHT2_GND       8   /**< Земля светодиода 2 */
  #define WORK_LIGHT_CURRENT_MA 20  /**< Номинальный ток одного светодиода, мА */
#endif
/** @} */

/** @defgroup status_led Статусный светодиод отладочной платы
 *  @{
 */
#define HAS_STATUS_LED          0   /**< 0 — не используется, 1 — активен */
#if HAS_STATUS_LED
  #define STATUS_LED_PIN        38  /**< Пин светодиода */
#endif
/** @} */

/** @defgroup timings Тайминги кнопок
 *  @{
 */
#define BUTTON_DEBOUNCE_MS      50  /**< Длительность антидребезга, мс */
#define BTN_LONG_PRESS_MS       300 /**< Порог удержания для длинного нажатия, мс */
/** @} */

/** @defgroup mouse HID-мышь
 *  @{
 */
#define MOUSE_UPDATE_HZ         125  /**< Частота обновления, Гц */
#define MOUSE_CURVE_EXPONENT    2.5f /**< Экспонента кривой ускорения */
/** @} */

#endif // CONFIG_H