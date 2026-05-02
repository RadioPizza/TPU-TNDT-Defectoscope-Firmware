#include <Arduino.h>
#include "config.h"
#include "Calibration.h"
#include "Joystick.h"
#include "MouseHID.h"

Calibration calibration;
Joystick joystick;
MouseHID mouse;

// Функции калибровки
void readJoystickAvg(int samples, float &avgX, float &avgY, float &stdX, float &stdY) {
    double sumX = 0, sumY = 0, sumX2 = 0, sumY2 = 0;
    for (int i = 0; i < samples; i++) {
        float x = (float)analogRead(JOY_X_PIN);
        float y = (float)analogRead(JOY_Y_PIN);
        sumX += x; sumY += y;
        sumX2 += x * x; sumY2 += y * y;
        delay(5);
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
    Serial.println("  [сбор данных...]");
    readJoystickAvg(samples, avgX, avgY, stdX, stdY);
}

bool waitMotionSettle(float &avgX, float &avgY) {
    int state = 0;
    unsigned long startTime = millis();
    float prevX = (float)analogRead(JOY_X_PIN);
    float prevY = (float)analogRead(JOY_Y_PIN);
    while (millis() - startTime < 15000) {
        delay(300);
        float nowX = (float)analogRead(JOY_X_PIN);
        float nowY = (float)analogRead(JOY_Y_PIN);
        float dx = nowX - prevX;
        float dy = nowY - prevY;
        float dist = sqrtf(dx * dx + dy * dy);
        prevX = nowX; prevY = nowY;
        switch (state) {
            case 0:
                if (dist > 80.0f) {
                    state = 1;
                    Serial.println("\n  [движение обнаружено...]");
                } else {
                    Serial.printf("  ожидание  dist=%.0f   \r", dist);
                }
                break;
            case 1:
                if (dist < 15.0f) {
                    Serial.println("\n  [остановка, снимаю данные...]");
                    delay(200);
                    float sx, sy;
                    readJoystickAvg(30, avgX, avgY, sx, sy);
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

float measureStickTime() {
    while (joystick.getMagnitude() > 0.15f) delay(5);
    delay(100);
    Serial.println("  Отводите!");
    while (joystick.getMagnitude() < calibration.data.deadZone + 0.05f) delay(2);
    unsigned long t0 = micros();
    while (joystick.getMagnitude() < 0.9f) {
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

void calibrate() {
    Serial.println("\n========================================");
    Serial.println("       КАЛИБРОВКА ДЖОЙСТИКА");
    Serial.println("========================================\n");

    // Шаг 1: Центр
    Serial.println("-- Шаг 1/5: Центр --");
    Serial.println("  Отпустите джойстик, нажмите кнопку.");
    float cx, cy, sx, sy;
    waitButtonThenCollect(NOISE_SAMPLES, cx, cy, sx, sy);
    calibration.data.centerX = cx;
    calibration.data.centerY = cy;
    calibration.data.noiseSigmaX = sx;
    calibration.data.noiseSigmaY = sy;
    Serial.printf("  Центр: (%.1f, %.1f) σ=(%.2f, %.2f)\n", cx, cy, sx, sy);

    delay(1000);
    float cx2, cy2, sx2, sy2;
    readJoystickAvg(NOISE_SAMPLES, cx2, cy2, sx2, sy2);
    float driftX = fabsf(cx2 - cx);
    float driftY = fabsf(cy2 - cy);
    Serial.printf("  Дрифт: (%.2f, %.2f)\n", driftX, driftY);
    calibration.data.centerX = (cx + cx2) / 2.0f;
    calibration.data.centerY = (cy + cy2) / 2.0f;
    float maxSigma = max(sx, sy);
    float deadADC = max(4.0f * maxSigma, 15.0f); // DEADZONE_SIGMA_MULT=4.0
    deadADC += max(driftX, driftY);
    Serial.printf("  Мёртвая зона (ADC): %.1f\n", deadADC);

    // Шаг 2: Ось X
    Serial.println("\n-- Шаг 2/5: Направление ВПРАВО --");
    Serial.println("  Нажмите кнопку, затем отведите ВПРАВО до упора.");
    waitButton();
    float rightX, rightY;
    if (!waitMotionSettle(rightX, rightY)) return;
    float rdx = rightX - calibration.data.centerX;
    float rdy = rightY - calibration.data.centerY;
    float rLen = sqrtf(rdx * rdx + rdy * rdy);
    if (rLen < 10.0f) { Serial.println("  ✗ Слишком малое отклонение!"); return; }
    calibration.data.axXx = rdx / rLen;
    calibration.data.axXy = rdy / rLen;
    calibration.data.angleDeg = atan2f(rdy, rdx) * 180.0f / PI;
    Serial.printf("  Угол монтажа: %.1f°\n", calibration.data.angleDeg);

    // Шаг 3: Ось Y
    Serial.println("\n-- Шаг 3/5: Направление ВВЕРХ --");
    Serial.println("  Нажмите кнопку, затем отведите ВВЕРХ до упора.");
    waitButton();
    float upX, upY;
    if (!waitMotionSettle(upX, upY)) return;
    float udx = upX - calibration.data.centerX;
    float udy = upY - calibration.data.centerY;
    float candAx = -calibration.data.axXy, candAy = calibration.data.axXx;
    float proj = udx * candAx + udy * candAy;
    if (proj >= 0) {
        calibration.data.axYx = candAx;
        calibration.data.axYy = candAy;
    } else {
        calibration.data.axYx = -candAx;
        calibration.data.axYy = -candAy;
    }
    Serial.printf("  Ось Y: (%.4f, %.4f)\n", calibration.data.axYx, calibration.data.axYy);

    // Шаг 4: Граница
    Serial.println("\n-- Шаг 4/5: Карта границы --");
    Serial.println("  Нажмите кнопку, затем вращайте по кругу в крайнем положении.");
    waitButton();
    float segData[SEGMENTS][5];
    int segCount[SEGMENTS];
    for (int i = 0; i < SEGMENTS; i++) segCount[i] = 0;
    int filledSegments = 0;
    unsigned long startTime = millis();
    while (filledSegments < SEGMENTS && (millis() - startTime) < 20000) {
        float rawX = (float)analogRead(JOY_X_PIN);
        float rawY = (float)analogRead(JOY_Y_PIN);
        float dx = rawX - calibration.data.centerX;
        float dy = rawY - calibration.data.centerY;
        float ux = dx * calibration.data.axXx + dy * calibration.data.axXy;
        float uy = dx * calibration.data.axYx + dy * calibration.data.axYy;
        float radius = sqrtf(ux * ux + uy * uy);
        if (radius < ADC_MAX * 0.15f) { delay(20); continue; }
        float angle = atan2f(uy, ux);
        if (angle < 0) angle += 2.0f * PI;
        int seg = (int)(angle / (2.0f * PI) * SEGMENTS);
        if (seg >= SEGMENTS) seg = SEGMENTS - 1;
        if (segCount[seg] < 5) {
            segData[seg][segCount[seg]++] = radius;
            if (segCount[seg] == 5) filledSegments++;
        }
        delay(20);
    }
    for (int i = 0; i < SEGMENTS; i++) {
        if (segCount[i] > 0) {
            float sum = 0;
            for (int j = 0; j < segCount[i]; j++) sum += segData[i][j];
            calibration.data.boundary[i] = sum / segCount[i];
        } else {
            calibration.data.boundary[i] = -1;
        }
    }
    // интерполяция (сокращённо)
    float avgB = 0;
    for (int i = 0; i < SEGMENTS; i++) if (calibration.data.boundary[i] < 0) calibration.data.boundary[i] = ADC_MAX/2.0f;
    for (int i = 0; i < SEGMENTS; i++) avgB += calibration.data.boundary[i];
    avgB /= SEGMENTS;
    calibration.data.deadZone = deadADC / avgB;

    calibration.data.valid = true;
    joystick.begin(calibration);

    // Шаг 5: Скорость
    Serial.println("\n-- Шаг 5/5: Скорость стика --");
    float slowSum = 0; int slowOk = 0;
    for (int i = 0; i < 3; i++) {
        waitButton();
        float t = measureStickTime();
        if (t > 0) { slowSum += t; slowOk++; }
    }
    calibration.data.slowTime = slowOk ? slowSum / slowOk : 600;
    float fastSum = 0; int fastOk = 0;
    for (int i = 0; i < 3; i++) {
        waitButton();
        float t = measureStickTime();
        if (t > 0) { fastSum += t; fastOk++; }
    }
    calibration.data.fastTime = fastOk ? fastSum / fastOk : 150;
    if (calibration.data.fastTime < 1) calibration.data.fastTime = 1;
    calibration.data.maxBoost = calibration.data.slowTime / calibration.data.fastTime;
    if (calibration.data.maxBoost < 1.0f) calibration.data.maxBoost = 1.0f;
    if (calibration.data.maxBoost > 10.0f) calibration.data.maxBoost = 10.0f;

    calibration.save(calibration.data);
    Serial.println("\nКалибровка завершена!");
}

unsigned long lastStatus = 0;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n=== ESP32-S3 Joystick Mouse ===");
    pinMode(JOY_BTN_PIN, INPUT_PULLUP);
    mouse.begin();
    if (calibration.load()) {
        Serial.println("Калибровка загружена");
        joystick.begin(calibration);
        Preferences prefs;
        prefs.begin("joycal", true);
        mouse.setSensitivity(prefs.getFloat("sens", 800.0f));
        prefs.end();
    } else {
        Serial.println("Калибровка отсутствует — мышь неактивна");
    }
    Serial.println("Команды: C(калибровка), R(сброс), I(инфо), S<число>");
}

void loop() {
    if (Serial.available()) {
        char cmd = Serial.read();
        if (cmd == 'S' || cmd == 's') {
            delay(50);
            String val;
            while (Serial.available()) val += (char)Serial.read();
            float s = val.toFloat();
            if (s > 0) {
                mouse.setSensitivity(s);
                Preferences prefs;
                prefs.begin("joycal", false);
                prefs.putFloat("sens", s);
                prefs.end();
                Serial.printf("Чувствительность: %.0f\n", s);
            }
        } else {
            while (Serial.available()) Serial.read();
            switch (cmd) {
                case 'C': case 'c': calibrate(); break;
                case 'R': case 'r':
                    calibration.clear();
                    joystick = Joystick();
                    Serial.println("Калибровка удалена");
                    break;
                case 'I': case 'i':
                    if (calibration.data.valid) {
                        Serial.println("Калибровка активна:");
                        Serial.printf("Центр: (%.1f, %.1f) Угол: %.1f\n", calibration.data.centerX, calibration.data.centerY, calibration.data.angleDeg);
                        Serial.printf("Мёртвая зона: %.3f\n", calibration.data.deadZone);
                        Serial.printf("Скорость: slow=%.0f fast=%.0f boost=%.1f\n", calibration.data.slowTime, calibration.data.fastTime, calibration.data.maxBoost);
                    }
                    break;
            }
        }
    }

    if (calibration.data.valid) {
        float jx, jy;
        joystick.process(jx, jy);
        float mag = sqrtf(jx * jx + jy * jy);
        joystick.updateBoost(mag);
        mouse.update(jx, jy, joystick.currentBoost);
        mouse.handleButton(digitalRead(JOY_BTN_PIN) == LOW);
    }

    if (millis() - lastStatus >= 1000) {
        lastStatus = millis();
        if (calibration.data.valid) {
            float x, y;
            joystick.process(x, y);
            Serial.printf("X: %.3f Y: %.3f boost: %.1f btn: %s\n", x, y, joystick.currentBoost, digitalRead(JOY_BTN_PIN)==LOW?"нажата":"отжата");
        }
    }
}