#include <Preferences.h>
#include <math.h>
#include "USB.h"
#include "USBHIDMouse.h"

USBHIDMouse Mouse;

// ======================= НАСТРОЙКИ ПИНОВ =======================
#define JOY_X_PIN   4
#define JOY_Y_PIN   5
#define JOY_BTN_PIN 6

// ======================= КОНСТАНТЫ =============================
#define ADC_BITS        12
#define ADC_MAX         ((1 << ADC_BITS) - 1)

// Калибровка
#define NOISE_SAMPLES       200
#define CAL_SAMPLE_DELAY_MS 5
#define DEADZONE_SIGMA_MULT 4.0f
#define DEADZONE_MIN        15.0f

// Детектор движения
#define MOTION_INTERVAL_MS  300
#define MOTION_THRESHOLD    80.0f
#define MOTION_STOP_THRESH  15.0f
#define SETTLE_SAMPLES      30

// Граница
#define SEGMENTS            36
#define SEG_POINTS          5
#define BOUNDARY_TIMEOUT_MS 20000

// Мышь
#define MOUSE_UPDATE_HZ     125             // Частота обновления USB HID
#define MOUSE_UPDATE_US     (1000000 / MOUSE_UPDATE_HZ)
#define CURVE_EXPONENT      2.5f            // Степень кривой скорости
#define FLICK_THRESHOLD     0.9f            // Порог «крайнее положение» для измерения скорости стика
#define BTN_LONG_PRESS_MS   300             // Порог длинного нажатия (ПКМ)
#define SPEED_MEASURE_TRIES 3               // Замеров при калибровке скорости

// ======================= СТРУКТУРА КАЛИБРОВКИ ==================
struct JoystickCalibration {
    float centerX, centerY;
    float axXx, axXy;
    float axYx, axYy;
    float boundary[SEGMENTS];
    float deadZone;
    float noiseSigmaX, noiseSigmaY;
    float angleDeg;

    // Калибровка скорости стика
    float slowTime;     // Время медленного отведения (мс)
    float fastTime;     // Время быстрого отведения (мс)
    float maxBoost;     // slowTime / fastTime

    bool valid;
};

// ======================= ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ =================
JoystickCalibration cal;
Preferences prefs;

// Настройки мыши (изменяемые через Serial)
float baseSensitivity = 800.0f;   // Пикселей/сек при полном отклонении (обычная скорость)

// Состояние мыши
float accX = 0, accY = 0;              // Накопитель дробных пикселей
unsigned long lastMouseUpdate = 0;

// Boost: измерение скорости стика в реальном времени
bool stickWasInCenter = true;           // Стик был в центре?
unsigned long stickExitTime = 0;        // Время выхода из мёртвой зоны
float currentBoost = 1.0f;             // Текущий множитель ускорения
bool boostMeasured = false;             // Boost уже измерен для этого движения?

// Состояние кнопки
enum BtnState { BTN_IDLE, BTN_WAIT, BTN_RMB_HELD };
BtnState btnState = BTN_IDLE;
unsigned long btnPressTime = 0;

// ======================= ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ===============

void readJoystickAvg(int samples, float &avgX, float &avgY, float &stdX, float &stdY) {
    double sumX = 0, sumY = 0, sumX2 = 0, sumY2 = 0;
    for (int i = 0; i < samples; i++) {
        float x = (float)analogRead(JOY_X_PIN);
        float y = (float)analogRead(JOY_Y_PIN);
        sumX += x; sumY += y;
        sumX2 += x * x; sumY2 += y * y;
        delay(CAL_SAMPLE_DELAY_MS);
    }
    avgX = (float)(sumX / samples);
    avgY = (float)(sumY / samples);
    float varX = (float)(sumX2 / samples) - avgX * avgX;
    float varY = (float)(sumY2 / samples) - avgY * avgY;
    stdX = (varX > 0) ? sqrtf(varX) : 0;
    stdY = (varY > 0) ? sqrtf(varY) : 0;
}

void waitButton() {
    while (digitalRead(JOY_BTN_PIN) == LOW) delay(10);
    delay(50);
    while (digitalRead(JOY_BTN_PIN) == HIGH) delay(10);
    while (digitalRead(JOY_BTN_PIN) == LOW) delay(10);
    delay(200);
}

void waitButtonThenCollect(int samples, float &avgX, float &avgY, float &stdX, float &stdY) {
    waitButton();
    Serial.println("  [пауза 2 сек...]");
    delay(2000);
    Serial.println("  [собираю данные...]");
    readJoystickAvg(samples, avgX, avgY, stdX, stdY);
}

bool waitMotionSettle(float &avgX, float &avgY) {
    int state = 0;
    unsigned long startTime = millis();
    float prevX = (float)analogRead(JOY_X_PIN);
    float prevY = (float)analogRead(JOY_Y_PIN);

    while (millis() - startTime < 15000) {
        delay(MOTION_INTERVAL_MS);
        float nowX = (float)analogRead(JOY_X_PIN);
        float nowY = (float)analogRead(JOY_Y_PIN);
        float dx = nowX - prevX;
        float dy = nowY - prevY;
        float dist = sqrtf(dx * dx + dy * dy);
        prevX = nowX;
        prevY = nowY;

        switch (state) {
            case 0:
                if (dist > MOTION_THRESHOLD) {
                    state = 1;
                    Serial.println("\n  [движение обнаружено...]");
                } else {
                    Serial.printf("  ожидание  dist=%.0f   \r", dist);
                }
                break;
            case 1:
                if (dist < MOTION_STOP_THRESH) {
                    Serial.println("\n  [остановка, снимаю данные...]");
                    delay(200);
                    float sx, sy;
                    readJoystickAvg(SETTLE_SAMPLES, avgX, avgY, sx, sy);
                    return true;
                } else {
                    Serial.printf("  движение  dist=%.0f   \r", dist);
                }
                break;
        }
    }
    Serial.println("\n  ✗ Таймаут!");
    return false;
}

/// Получить нормализованную величину отклонения (0..1+) в реальном времени
float getJoystickMagnitude() {
    if (!cal.valid) return 0;
    float rawX = (float)analogRead(JOY_X_PIN);
    float rawY = (float)analogRead(JOY_Y_PIN);
    float dx = rawX - cal.centerX;
    float dy = rawY - cal.centerY;
    float ux = dx * cal.axXx + dy * cal.axXy;
    float uy = dx * cal.axYx + dy * cal.axYy;
    float radius = sqrtf(ux * ux + uy * uy);
    float angle = atan2f(uy, ux);
    float boundaryR = getBoundaryRadius(angle);
    return radius / boundaryR;
}

// ======================= СОХРАНЕНИЕ / ЗАГРУЗКА =================

void saveCalibration() {
    prefs.begin("joycal", false);
    prefs.putFloat("cx", cal.centerX);
    prefs.putFloat("cy", cal.centerY);
    prefs.putFloat("axXx", cal.axXx);
    prefs.putFloat("axXy", cal.axXy);
    prefs.putFloat("axYx", cal.axYx);
    prefs.putFloat("axYy", cal.axYy);
    prefs.putFloat("dz", cal.deadZone);
    prefs.putFloat("nsX", cal.noiseSigmaX);
    prefs.putFloat("nsY", cal.noiseSigmaY);
    prefs.putFloat("ang", cal.angleDeg);
    prefs.putFloat("slT", cal.slowTime);
    prefs.putFloat("fsT", cal.fastTime);
    prefs.putFloat("mxB", cal.maxBoost);
    prefs.putBytes("bnd", cal.boundary, sizeof(cal.boundary));
    prefs.putBool("valid", true);
    prefs.end();
    Serial.println("  Калибровка сохранена в NVS");
}

bool loadCalibration() {
    prefs.begin("joycal", true);
    cal.valid = prefs.getBool("valid", false);
    if (!cal.valid) { prefs.end(); return false; }
    cal.centerX     = prefs.getFloat("cx", 0);
    cal.centerY     = prefs.getFloat("cy", 0);
    cal.axXx        = prefs.getFloat("axXx", 1);
    cal.axXy        = prefs.getFloat("axXy", 0);
    cal.axYx        = prefs.getFloat("axYx", 0);
    cal.axYy        = prefs.getFloat("axYy", 1);
    cal.deadZone    = prefs.getFloat("dz", 0.02);
    cal.noiseSigmaX = prefs.getFloat("nsX", 0);
    cal.noiseSigmaY = prefs.getFloat("nsY", 0);
    cal.angleDeg    = prefs.getFloat("ang", 0);
    cal.slowTime    = prefs.getFloat("slT", 600);
    cal.fastTime    = prefs.getFloat("fsT", 150);
    cal.maxBoost    = prefs.getFloat("mxB", 4.0);
    size_t len = prefs.getBytes("bnd", cal.boundary, sizeof(cal.boundary));
    if (len != sizeof(cal.boundary)) {
        for (int i = 0; i < SEGMENTS; i++) cal.boundary[i] = (float)ADC_MAX / 2.0f;
    }
    prefs.end();
    Serial.println("Калибровка загружена:");
    Serial.printf("  Центр: (%.1f, %.1f)  Угол: %.1f°\n", cal.centerX, cal.centerY, cal.angleDeg);
    Serial.printf("  Мёртвая зона: %.3f\n", cal.deadZone);
    Serial.printf("  Скорость стика: slow=%.0fмс fast=%.0fмс boost=%.1fx\n",
                  cal.slowTime, cal.fastTime, cal.maxBoost);
    return true;
}

void clearCalibration() {
    prefs.begin("joycal", false);
    prefs.clear();
    prefs.end();
    cal.valid = false;
    Serial.println("Калибровка удалена");
}

// ======================= ОБРАБОТКА СИГНАЛА =====================

float getBoundaryRadius(float angle) {
    if (angle < 0) angle += 2.0f * (float)M_PI;
    if (angle >= 2.0f * (float)M_PI) angle -= 2.0f * (float)M_PI;
    float segF = angle / (2.0f * (float)M_PI) * SEGMENTS;
    int seg0 = (int)segF;
    int seg1 = (seg0 + 1) % SEGMENTS;
    float frac = segF - seg0;
    if (seg0 >= SEGMENTS) seg0 = SEGMENTS - 1;
    return cal.boundary[seg0] * (1.0f - frac) + cal.boundary[seg1] * frac;
}

/// Основная обработка: выход -1.000 .. +1.000
void processJoystick(float &outX, float &outY) {
    if (!cal.valid) {
        outX = (float)analogRead(JOY_X_PIN);
        outY = (float)analogRead(JOY_Y_PIN);
        return;
    }
    float rawX = (float)analogRead(JOY_X_PIN);
    float rawY = (float)analogRead(JOY_Y_PIN);
    float dx = rawX - cal.centerX;
    float dy = rawY - cal.centerY;
    float ux = dx * cal.axXx + dy * cal.axXy;
    float uy = dx * cal.axYx + dy * cal.axYy;
    float radius = sqrtf(ux * ux + uy * uy);

    if (radius < 0.001f) { outX = 0; outY = 0; return; }

    float angle = atan2f(uy, ux);
    float boundaryR = getBoundaryRadius(angle);
    float norm = radius / boundaryR;

    if (norm < cal.deadZone) { outX = 0; outY = 0; return; }

    float remapped = (norm - cal.deadZone) / (1.0f - cal.deadZone);
    if (remapped > 1.0f) remapped = 1.0f;

    float dirX = ux / radius;
    float dirY = uy / radius;
    outX = dirX * remapped;
    outY = dirY * remapped;
}

// ======================= КАЛИБРОВКА ============================

/// Измеряет время отведения стика от центра до 0.9
/// Требует предварительно откалиброванных осей и границы
float measureStickTime() {
    // Ждём пока стик в центре
    while (getJoystickMagnitude() > 0.15f) delay(5);
    delay(100);

    Serial.println("  Отводите!");

    // Ждём выхода из мёртвой зоны
    while (getJoystickMagnitude() < cal.deadZone + 0.05f) delay(2);
    unsigned long t0 = micros();

    // Ждём достижения 0.9
    while (getJoystickMagnitude() < FLICK_THRESHOLD) {
        if (micros() - t0 > 3000000) {
            Serial.println("  ✗ Таймаут (3 сек)");
            return -1;
        }
        delayMicroseconds(500);
    }
    unsigned long t1 = micros();
    float timeMs = (t1 - t0) / 1000.0f;
    Serial.printf("  Время: %.0f мс\n", timeMs);
    return timeMs;
}

bool calibrate() {
    Serial.println();
    Serial.println("========================================");
    Serial.println("       КАЛИБРОВКА ДЖОЙСТИКА");
    Serial.println("========================================");
    Serial.println();

    cal.valid = false;

    // ─── Шаг 1: Центр ─────────────────────────────────────────
    Serial.println("-- Шаг 1/5: Определение центра --");
    Serial.println("  Отпустите джойстик. Нажмите кнопку.");
    waitButtonThenCollect(NOISE_SAMPLES, cal.centerX, cal.centerY, cal.noiseSigmaX, cal.noiseSigmaY);
    Serial.printf("  Центр:  X = %.1f   Y = %.1f\n", cal.centerX, cal.centerY);
    Serial.printf("  Шум σ:  X = %.2f   Y = %.2f\n", cal.noiseSigmaX, cal.noiseSigmaY);

    Serial.println("  Проверка дрифта (1 сек)...");
    delay(1000);
    float cx2, cy2, sx2, sy2;
    readJoystickAvg(NOISE_SAMPLES, cx2, cy2, sx2, sy2);
    float driftX = fabsf(cx2 - cal.centerX);
    float driftY = fabsf(cy2 - cal.centerY);
    Serial.printf("  Дрифт:  dX = %.2f   dY = %.2f\n", driftX, driftY);
    if (driftX > 20 || driftY > 20) Serial.println("  ⚠ Заметный дрифт!");
    else Serial.println("  ✓ Дрифт в норме");

    cal.centerX = (cal.centerX + cx2) / 2.0f;
    cal.centerY = (cal.centerY + cy2) / 2.0f;
    float maxSigma = max(cal.noiseSigmaX, cal.noiseSigmaY);
    float deadZoneADC = max(DEADZONE_SIGMA_MULT * maxSigma, DEADZONE_MIN);
    deadZoneADC += max(driftX, driftY);
    Serial.printf("  Мёртвая зона (ADC): %.1f\n", deadZoneADC);
    Serial.println();

    // ─── Шаг 2: Направление X ─────────────────────────────────
    Serial.println("-- Шаг 2/5: Направление ВПРАВО --");
    Serial.println("  Нажмите кнопку, затем отведите ВПРАВО до упора.");
    waitButton();
    Serial.println("  Отводите...");

    float rightX, rightY;
    if (!waitMotionSettle(rightX, rightY)) return false;
    float rdx = rightX - cal.centerX;
    float rdy = rightY - cal.centerY;
    float rLen = sqrtf(rdx * rdx + rdy * rdy);
    if (rLen < 10.0f) { Serial.println("  ✗ Отклонение слишком мало!"); return false; }

    cal.axXx = rdx / rLen;
    cal.axXy = rdy / rLen;
    cal.angleDeg = atan2f(rdy, rdx) * 180.0f / (float)M_PI;
    Serial.printf("  Угол монтажа: %.1f°\n", cal.angleDeg);
    Serial.println("  ✓ Ось X определена\n");

    // ─── Шаг 3: Знак оси Y ────────────────────────────────────
    Serial.println("-- Шаг 3/5: Направление ВВЕРХ --");
    Serial.println("  Нажмите кнопку, затем отведите ВВЕРХ до упора.");
    waitButton();
    Serial.println("  Отводите...");

    float upX, upY;
    if (!waitMotionSettle(upX, upY)) return false;
    float udx = upX - cal.centerX;
    float udy = upY - cal.centerY;
    float candAx = -cal.axXy, candAy = cal.axXx;
    float proj = udx * candAx + udy * candAy;
    if (proj >= 0) { cal.axYx = candAx; cal.axYy = candAy; }
    else           { cal.axYx = -candAx; cal.axYy = -candAy; }
    Serial.printf("  Ось Y: (%.4f, %.4f)\n", cal.axYx, cal.axYy);
    Serial.println("  ✓ Ось Y определена\n");

    // ─── Шаг 4: Карта границы ─────────────────────────────────
    Serial.println("-- Шаг 4/5: Карта границы --");
    Serial.println("  Нажмите кнопку, затем вращайте по кругу в крайнем положении.");
    waitButton();
    Serial.println("  Вращайте...\n");

    float segData[SEGMENTS][SEG_POINTS];
    int segCount[SEGMENTS];
    for (int i = 0; i < SEGMENTS; i++) segCount[i] = 0;
    int filledSegments = 0;
    unsigned long startTime = millis();
    unsigned long lastProgressTime = 0;

    while (filledSegments < SEGMENTS && (millis() - startTime) < BOUNDARY_TIMEOUT_MS) {
        float rawX = (float)analogRead(JOY_X_PIN);
        float rawY = (float)analogRead(JOY_Y_PIN);
        float dx = rawX - cal.centerX;
        float dy = rawY - cal.centerY;
        float ux = dx * cal.axXx + dy * cal.axXy;
        float uy = dx * cal.axYx + dy * cal.axYy;
        float radius = sqrtf(ux * ux + uy * uy);

        if (radius < (float)ADC_MAX * 0.15f) { delay(20); continue; }

        float angle = atan2f(uy, ux);
        if (angle < 0) angle += 2.0f * (float)M_PI;
        int seg = (int)(angle / (2.0f * (float)M_PI) * SEGMENTS);
        if (seg >= SEGMENTS) seg = SEGMENTS - 1;

        if (segCount[seg] < SEG_POINTS) {
            segData[seg][segCount[seg]] = radius;
            segCount[seg]++;
            if (segCount[seg] == SEG_POINTS) filledSegments++;
        }

        if (millis() - lastProgressTime > 500) {
            lastProgressTime = millis();
            Serial.printf("  Сегменты: %d/%d  [", filledSegments, SEGMENTS);
            for (int i = 0; i < SEGMENTS; i++) {
                if (segCount[i] >= SEG_POINTS) Serial.print("#");
                else if (segCount[i] > 0) Serial.print(".");
                else Serial.print(" ");
            }
            Serial.println("]");
        }
        delay(20);
    }
    Serial.println();

    for (int i = 0; i < SEGMENTS; i++) {
        if (segCount[i] > 0) {
            float sum = 0;
            for (int j = 0; j < segCount[i]; j++) sum += segData[i][j];
            cal.boundary[i] = sum / segCount[i];
        } else {
            cal.boundary[i] = -1;
        }
    }

    if (filledSegments < SEGMENTS) {
        Serial.printf("  ⚠ Заполнено %d/%d, интерполирую\n", filledSegments, SEGMENTS);
        for (int i = 0; i < SEGMENTS; i++) {
            if (cal.boundary[i] >= 0) continue;
            int left = -1, right = -1;
            for (int d = 1; d < SEGMENTS; d++) {
                int li = (i - d + SEGMENTS) % SEGMENTS;
                int ri = (i + d) % SEGMENTS;
                if (left < 0 && cal.boundary[li] >= 0) left = li;
                if (right < 0 && cal.boundary[ri] >= 0) right = ri;
                if (left >= 0 && right >= 0) break;
            }
            if (left >= 0 && right >= 0) {
                int distL = (i - left + SEGMENTS) % SEGMENTS;
                int distR = (right - i + SEGMENTS) % SEGMENTS;
                float t = (float)distL / (float)(distL + distR);
                cal.boundary[i] = cal.boundary[left] * (1.0f - t) + cal.boundary[right] * t;
            } else if (left >= 0) cal.boundary[i] = cal.boundary[left];
            else if (right >= 0) cal.boundary[i] = cal.boundary[right];
            else cal.boundary[i] = (float)ADC_MAX / 2.0f;
        }
    } else {
        Serial.println("  ✓ Все сегменты заполнены");
    }

    float avgBoundary = 0;
    for (int i = 0; i < SEGMENTS; i++) avgBoundary += cal.boundary[i];
    avgBoundary /= SEGMENTS;
    cal.deadZone = deadZoneADC / avgBoundary;

    // Временно помечаем калибровку валидной для measureStickTime
    cal.valid = true;

    Serial.println("  Карта границы:");
    for (int i = 0; i < SEGMENTS; i++)
        Serial.printf("    %3d°: %.0f\n", i * (360 / SEGMENTS), cal.boundary[i]);
    Serial.printf("  Мёртвая зона: %.3f\n", cal.deadZone);
    Serial.println();

    // ─── Шаг 5: Калибровка скорости стика ──────────────────────
    Serial.println("-- Шаг 5/5: Скорость стика --");
    Serial.println();

    // Медленное отведение
    Serial.println("  Сейчас МЕДЛЕННО отведите стик до упора.");
    Serial.printf("  Нужно %d замеров. Нажмите кнопку перед каждым.\n", SPEED_MEASURE_TRIES);
    float slowSum = 0;
    int slowOk = 0;
    for (int i = 0; i < SPEED_MEASURE_TRIES; i++) {
        Serial.printf("  Замер %d/%d — нажмите кнопку, затем МЕДЛЕННО отведите.\n", i + 1, SPEED_MEASURE_TRIES);
        waitButton();
        float t = measureStickTime();
        if (t > 0) { slowSum += t; slowOk++; }
    }

    if (slowOk == 0) {
        Serial.println("  ✗ Не удалось измерить медленную скорость!");
        cal.slowTime = 600;
    } else {
        cal.slowTime = slowSum / slowOk;
    }
    Serial.printf("  Среднее медленное: %.0f мс\n\n", cal.slowTime);

    // Быстрое отведение
    Serial.println("  Теперь БЫСТРО (рывком) отведите стик до упора.");
    Serial.printf("  Нужно %d замеров. Нажмите кнопку перед каждым.\n", SPEED_MEASURE_TRIES);
    float fastSum = 0;
    int fastOk = 0;
    for (int i = 0; i < SPEED_MEASURE_TRIES; i++) {
        Serial.printf("  Замер %d/%d — нажмите кнопку, затем БЫСТРО отведите.\n", i + 1, SPEED_MEASURE_TRIES);
        waitButton();
        float t = measureStickTime();
        if (t > 0) { fastSum += t; fastOk++; }
    }

    if (fastOk == 0) {
        Serial.println("  ✗ Не удалось измерить быструю скорость!");
        cal.fastTime = 150;
    } else {
        cal.fastTime = fastSum / fastOk;
    }
    Serial.printf("  Среднее быстрое: %.0f мс\n", cal.fastTime);

    // Вычисляем максимальный множитель
    if (cal.fastTime < 1) cal.fastTime = 1;
    cal.maxBoost = cal.slowTime / cal.fastTime;
    if (cal.maxBoost < 1.0f) cal.maxBoost = 1.0f;
    if (cal.maxBoost > 10.0f) cal.maxBoost = 10.0f;

    Serial.printf("  Множитель ускорения: %.1fx\n", cal.maxBoost);

    saveCalibration();

    Serial.println();
    Serial.println("========================================");
    Serial.println("       КАЛИБРОВКА ЗАВЕРШЕНА!");
    Serial.println("========================================");
    Serial.println();
    return true;
}

// ======================= МЫШЬ ==================================

/// Обновление boost-множителя на основе скорости стика
void updateBoost(float magnitude) {
    // magnitude — нормализованная длина вектора (0..1) из processJoystick

    if (magnitude < cal.deadZone + 0.02f) {
        // Стик в центре — сбрасываем
        stickWasInCenter = true;
        boostMeasured = false;
        currentBoost = 1.0f;
        return;
    }

    if (stickWasInCenter && !boostMeasured) {
        // Только что вышли из центра — засекаем время
        stickExitTime = micros();
        stickWasInCenter = false;
    }

    if (!boostMeasured && magnitude >= FLICK_THRESHOLD) {
        // Достигли крайнего положения — измеряем время
        float elapsed = (micros() - stickExitTime) / 1000.0f;  // мс
        if (elapsed < 1) elapsed = 1;

        // boost = slowTime / elapsed, ограничен диапазоном [1, maxBoost]
        currentBoost = cal.slowTime / elapsed;
        if (currentBoost < 1.0f) currentBoost = 1.0f;
        if (currentBoost > cal.maxBoost) currentBoost = cal.maxBoost;

        boostMeasured = true;
    }
}

/// Обновление кнопки мыши
void updateButton() {
    bool pressed = (digitalRead(JOY_BTN_PIN) == LOW);

    switch (btnState) {
        case BTN_IDLE:
            if (pressed) {
                btnPressTime = millis();
                btnState = BTN_WAIT;
            }
            break;

        case BTN_WAIT:
            if (!pressed) {
                // Отпущена быстро — ЛКМ клик
                Mouse.click(MOUSE_LEFT);
                btnState = BTN_IDLE;
            } else if (millis() - btnPressTime >= BTN_LONG_PRESS_MS) {
                // Удержание — ПКМ
                Mouse.press(MOUSE_RIGHT);
                btnState = BTN_RMB_HELD;
            }
            break;

        case BTN_RMB_HELD:
            if (!pressed) {
                Mouse.release(MOUSE_RIGHT);
                btnState = BTN_IDLE;
            }
            break;
    }
}

/// Основной цикл мыши (вызывается из loop с частотой MOUSE_UPDATE_HZ)
void updateMouse() {
    if (!cal.valid) return;

    float jx, jy;
    processJoystick(jx, jy);

    float mag = sqrtf(jx * jx + jy * jy);

    // Обновляем boost
    updateBoost(mag);

    if (mag < 0.001f) {
        accX = 0;
        accY = 0;
        return;
    }

    // Кривая скорости: mag^CURVE_EXPONENT
    // При mag=0.1 → 0.003, mag=0.5 → 0.177, mag=1.0 → 1.0
    float curved = powf(mag, CURVE_EXPONENT);

    // Направление (единичный вектор)
    float dirX = jx / mag;
    float dirY = jy / mag;

    // Скорость: базовая * кривая * boost
    float speed = baseSensitivity * curved * currentBoost;

    // Пиксели за один тик (1/125 сек)
    float dt = 1.0f / MOUSE_UPDATE_HZ;
    accX += dirX * speed * dt;
    accY += dirY * speed * dt;

    // Отправляем целую часть
    int moveX = (int)accX;
    int moveY = (int)accY;
    accX -= moveX;
    accY -= moveY;

    // Ограничиваем до int8_t
    if (moveX > 127) moveX = 127;
    if (moveX < -127) moveX = -127;
    if (moveY > 127) moveY = 127;
    if (moveY < -127) moveY = -127;

    if (moveX != 0 || moveY != 0) {
        Mouse.move(moveX, -moveY);  // Y инвертирован: в USB HID +Y = вниз
    }
}

// ======================= SETUP / LOOP ==========================

void setup() {
    Serial.begin(115200);
    delay(500);

    pinMode(JOY_BTN_PIN, INPUT_PULLUP);
    analogReadResolution(ADC_BITS);
    analogSetAttenuation(ADC_11db);

    // USB HID
    Mouse.begin();
    USB.begin();

    Serial.println();
    Serial.println("=== Joystick Mouse ===");

    if (loadCalibration()) {
        Serial.println("Режим: мышь активна");
    } else {
        Serial.println("Калибровка отсутствует — мышь выключена");
    }

    // Загружаем чувствительность из NVS
    prefs.begin("joycal", true);
    baseSensitivity = prefs.getFloat("sens", 800.0f);
    prefs.end();
    Serial.printf("Чувствительность: %.0f пикс/сек\n", baseSensitivity);

    Serial.println();
    Serial.println("Команды:");
    Serial.println("  C — калибровка");
    Serial.println("  R — сброс калибровки");
    Serial.println("  I — информация");
    Serial.println("  S<число> — чувствительность (пикс/сек), напр. S800");
    Serial.println();
}

unsigned long lastPrint = 0;

void loop() {
    // Команды Serial
    if (Serial.available()) {
        char cmd = Serial.read();

        if (cmd == 'S' || cmd == 's') {
            // Читаем число после S
            delay(50);
            String val = "";
            while (Serial.available()) val += (char)Serial.read();
            float newSens = val.toFloat();
            if (newSens > 0) {
                baseSensitivity = newSens;
                prefs.begin("joycal", false);
                prefs.putFloat("sens", baseSensitivity);
                prefs.end();
                Serial.printf("Чувствительность: %.0f пикс/сек (сохранено)\n", baseSensitivity);
            }
        } else {
            while (Serial.available()) Serial.read();
            switch (cmd) {
                case 'C': case 'c': calibrate(); break;
                case 'R': case 'r': clearCalibration(); break;
                case 'I': case 'i':
                    if (cal.valid) {
                        Serial.println("Калибровка активна:");
                        Serial.printf("  Центр: (%.1f, %.1f)  Угол: %.1f°\n", cal.centerX, cal.centerY, cal.angleDeg);
                        Serial.printf("  Ось X: (%.4f, %.4f)\n", cal.axXx, cal.axXy);
                        Serial.printf("  Ось Y: (%.4f, %.4f)\n", cal.axYx, cal.axYy);
                        Serial.printf("  Мёртвая зона: %.3f\n", cal.deadZone);
                        Serial.printf("  Скорость: slow=%.0fмс fast=%.0fмс boost=%.1fx\n",
                                      cal.slowTime, cal.fastTime, cal.maxBoost);
                        Serial.printf("  Чувствительность: %.0f пикс/сек\n", baseSensitivity);
                    } else {
                        Serial.println("Калибровка отсутствует");
                    }
                    break;
            }
        }
    }

    // Мышь: 125 Гц
    unsigned long now = micros();
    if (now - lastMouseUpdate >= MOUSE_UPDATE_US) {
        lastMouseUpdate = now;
        updateMouse();
        updateButton();
    }

    // Статус: 1 Гц
    if (millis() - lastPrint >= 1000) {
        lastPrint = millis();
        float x, y;
        processJoystick(x, y);
        bool btn = (digitalRead(JOY_BTN_PIN) == LOW);

        if (cal.valid) {
            float mag = sqrtf(x * x + y * y);
            Serial.printf("X: %.3f  Y: %.3f  boost: %.1f  Кнопка: %s\n",
                          x, y, currentBoost, btn ? "нажата" : "отжата");
        } else {
            Serial.printf("X: %.0f (raw)  Y: %.0f (raw)  Кнопка: %s\n",
                          x, y, btn ? "нажата" : "отжата");
        }
    }
}
