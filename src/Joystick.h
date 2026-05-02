#pragma once
#include <Arduino.h>
#include "config.h"
#include "Calibration.h"

class Joystick {
public:
    void begin(const Calibration &calib) {
        cal = &calib;
        analogReadResolution(ADC_BITS);
        analogSetAttenuation(ADC_11db);
    }

    void process(float &outX, float &outY) {
        outX = 0.0f; outY = 0.0f;
        if (!cal || !cal->data.valid) return;
        float rawX = (float)analogRead(JOY_X_PIN);
        float rawY = (float)analogRead(JOY_Y_PIN);
        float dx = rawX - cal->data.centerX;
        float dy = rawY - cal->data.centerY;
        float ux = dx * cal->data.axXx + dy * cal->data.axXy;
        float uy = dx * cal->data.axYx + dy * cal->data.axYy;
        float radius = sqrtf(ux * ux + uy * uy);
        if (radius < 0.001f) return;
        float angle = atan2f(uy, ux);
        float boundR = getBoundaryRadius(angle);
        float norm = radius / boundR;
        if (norm < cal->data.deadZone) return;
        float remapped = (norm - cal->data.deadZone) / (1.0f - cal->data.deadZone);
        if (remapped > 1.0f) remapped = 1.0f;
        float dirX = ux / radius;
        float dirY = uy / radius;
        outX = dirX * remapped;
        outY = dirY * remapped;
    }

    float getMagnitude() const {
        float x, y;
        const_cast<Joystick*>(this)->process(x, y);
        return sqrtf(x * x + y * y);
    }

    void updateBoost(float magnitude) {
        if (!cal || !cal->data.valid) return;
        if (magnitude < cal->data.deadZone + 0.02f) {
            stickWasInCenter = true;
            boostMeasured = false;
            currentBoost = 1.0f;
            return;
        }
        if (stickWasInCenter && !boostMeasured) {
            stickExitTime = micros();
            stickWasInCenter = false;
        }
        if (!boostMeasured && magnitude >= 0.9f) {
            float elapsed = (micros() - stickExitTime) / 1000.0f;
            if (elapsed < 1) elapsed = 1;
            currentBoost = cal->data.slowTime / elapsed;
            if (currentBoost < 1.0f) currentBoost = 1.0f;
            if (currentBoost > cal->data.maxBoost) currentBoost = cal->data.maxBoost;
            boostMeasured = true;
        }
    }

    float currentBoost = 1.0f;

private:
    const Calibration *cal = nullptr;
    bool stickWasInCenter = true;
    unsigned long stickExitTime = 0;
    bool boostMeasured = false;

    float getBoundaryRadius(float angle) const {
        if (!cal || !cal->data.valid) return ADC_MAX / 2.0f;
        if (angle < 0) angle += 2.0f * PI;
        float segF = angle / (2.0f * PI) * SEGMENTS;
        int seg0 = (int)segF;
        int seg1 = (seg0 + 1) % SEGMENTS;
        float frac = segF - seg0;
        if (seg0 >= SEGMENTS) seg0 = SEGMENTS - 1;
        return cal->data.boundary[seg0] * (1.0f - frac) + cal->data.boundary[seg1] * frac;
    }
};