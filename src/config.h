#ifndef CONFIG_H
#define CONFIG_H

// Версия прошивки контроллера
#define FW_VERSION_MAJOR 0
#define FW_VERSION_MINOR 1
#define FW_VERSION_PATCH 0

// Свойства нагревательного узла
#define HEATER_REVISION      2    // Ревизия узла
#define HEATER_LAMP_COUNT    4    // Количество нагревательных элементов
#define HEATER_POWER_WATTS   2000 // Общая мощность, Вт
#define HEATER_MAX_ON_TIME_S 10   // Макс. время непрерывной работы, с
#define HEATER_MIN_PAUSE_S   20   // Мин. пауза между импульсами, с

#endif