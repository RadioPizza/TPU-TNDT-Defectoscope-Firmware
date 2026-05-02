#pragma once
#include <USB.h>
#include <USBHIDMouse.h>
#include "config.h"

class MouseHID {
public:
    void begin() {
        mouse.begin();
        USB.begin();
    }

    void update(float joyX, float joyY, float boost) {
        unsigned long now = micros();
        if (now - lastUpdate < 1000000 / MOUSE_UPDATE_HZ) return;
        lastUpdate = now;

        float mag = sqrtf(joyX * joyX + joyY * joyY);
        if (mag < 0.001f) {
            accX = accY = 0;
            return;
        }

        float curved = powf(mag, CURVE_EXPONENT);
        float dirX = joyX / mag;
        float dirY = joyY / mag;
        float dt = 1.0f / MOUSE_UPDATE_HZ;
        float speed = sensitivity * curved * boost;
        accX += dirX * speed * dt;
        accY += dirY * speed * dt;

        int moveX = (int)accX;
        int moveY = (int)accY;
        accX -= moveX;
        accY -= moveY;

        if (moveX > 127) moveX = 127;
        if (moveX < -127) moveX = -127;
        if (moveY > 127) moveY = 127;
        if (moveY < -127) moveY = -127;

        if (moveX != 0 || moveY != 0) {
            mouse.move(moveX, -moveY); // HID Y ось направлена вниз
        }
    }

    void handleButton(bool pressed) {
        switch (btnState) {
            case BTN_IDLE:
                if (pressed) {
                    btnPressTime = millis();
                    btnState = BTN_WAIT;
                }
                break;
            case BTN_WAIT:
                if (!pressed) {
                    mouse.click(MOUSE_LEFT);
                    btnState = BTN_IDLE;
                } else if (millis() - btnPressTime >= BTN_LONG_PRESS_MS) {
                    mouse.press(MOUSE_RIGHT);
                    btnState = BTN_RMB_HELD;
                }
                break;
            case BTN_RMB_HELD:
                if (!pressed) {
                    mouse.release(MOUSE_RIGHT);
                    btnState = BTN_IDLE;
                }
                break;
        }
    }

    void setSensitivity(float sens) {
        sensitivity = sens;
    }

    float sensitivity = 800.0f;

private:
    USBHIDMouse mouse;
    float accX = 0, accY = 0;
    unsigned long lastUpdate = 0;
    enum BtnState { BTN_IDLE, BTN_WAIT, BTN_RMB_HELD };
    BtnState btnState = BTN_IDLE;
    unsigned long btnPressTime = 0;
};