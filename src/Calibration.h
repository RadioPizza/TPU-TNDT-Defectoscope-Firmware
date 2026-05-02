#pragma once
#include <Preferences.h>
#include <cmath>
#include "config.h"

#define SEGMENTS 36

struct JoystickCalibration {
    float centerX, centerY;
    float axXx, axXy, axYx, axYy;
    float boundary[SEGMENTS];
    float deadZone;
    float noiseSigmaX, noiseSigmaY;
    float angleDeg;
    float slowTime, fastTime, maxBoost;
    bool valid;
};

class Calibration {
public:
    bool load() {
        Preferences prefs;
        prefs.begin("joycal", true);
        data.valid = prefs.getBool("valid", false);
        if (!data.valid) { prefs.end(); return false; }
        data.centerX     = prefs.getFloat("cx", 0);
        data.centerY     = prefs.getFloat("cy", 0);
        data.axXx        = prefs.getFloat("axXx", 1);
        data.axXy        = prefs.getFloat("axXy", 0);
        data.axYx        = prefs.getFloat("axYx", 0);
        data.axYy        = prefs.getFloat("axYy", 1);
        data.deadZone    = prefs.getFloat("dz", 0.02);
        data.noiseSigmaX = prefs.getFloat("nsX", 0);
        data.noiseSigmaY = prefs.getFloat("nsY", 0);
        data.angleDeg    = prefs.getFloat("ang", 0);
        data.slowTime    = prefs.getFloat("slT", 600);
        data.fastTime    = prefs.getFloat("fsT", 150);
        data.maxBoost    = prefs.getFloat("mxB", 4.0);
        size_t len = prefs.getBytes("bnd", data.boundary, sizeof(data.boundary));
        if (len != sizeof(data.boundary)) {
            for (int i = 0; i < SEGMENTS; i++) data.boundary[i] = (float)ADC_MAX / 2.0f;
        }
        prefs.end();
        return true;
    }

    void save(const JoystickCalibration &cal) {
        Preferences prefs;
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
    }

    void clear() {
        Preferences prefs;
        prefs.begin("joycal", false);
        prefs.clear();
        prefs.end();
        data.valid = false;
    }

    JoystickCalibration data;
};