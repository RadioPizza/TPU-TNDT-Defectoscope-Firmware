/**
 * @file work_light_channel.cpp
 * @brief Реализация канала LED-подсветки с ШИМ-управлением.
 */

#include "work_light_channel.h"

WorkLightChannel::WorkLightChannel(uint8_t signalPin,
                                   uint8_t ledcChannel,
                                   int8_t  gndPin,
                                   gpio_drive_cap_t driveCap)
    : _signalPin(signalPin)
    , _ledcChannel(ledcChannel)
    , _gndPin(gndPin)
    , _driveCap(driveCap)
    , _state(false)
    , _brightness(0)
{}

void WorkLightChannel::begin() {
    // Настройка LEDC: частота, разрешение
    ledcSetup(_ledcChannel, LEDC_PWM_FREQ, LEDC_PWM_RESOLUTION);
    // Привязка пина к LEDC-каналу
    ledcAttachPin(_signalPin, _ledcChannel);

    // Установка токового режима (после того, как пин настроен как выход LEDC)
    // ledcAttachPin переключает пин в режим OUTPUT, можно сразу задать drive capability
    gpio_set_drive_capability((gpio_num_t)_signalPin, _driveCap);

    // Если предусмотрен отдельный земляной пин — сделать его цифровой землёй
    if (_gndPin >= 0) {
        pinMode(_gndPin, OUTPUT);
        digitalWrite(_gndPin, LOW);
    }

    // Гарантированно выключено
    ledcWrite(_ledcChannel, 0);
    _state = false;
    _brightness = 0;
}

void WorkLightChannel::turnOn() {
    _state = true;
    applyPwm();
}

void WorkLightChannel::turnOff() {
    _state = false;
    applyPwm();
}

void WorkLightChannel::setBrightness(uint8_t brightness) {
    _brightness = brightness;
    if (_state) {
        applyPwm();
    }
}

uint8_t WorkLightChannel::getBrightness() const {
    return _brightness;
}

bool WorkLightChannel::isOn() const {
    return _state;
}

void WorkLightChannel::applyPwm() {
    if (_state) {
        ledcWrite(_ledcChannel, _brightness);
    } else {
        ledcWrite(_ledcChannel, 0);
    }
}