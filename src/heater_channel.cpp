/**
 * @file heater_channel.cpp
 * @brief Реализация защищённого управления одним каналом нагревателя.
 *
 * Содержит логику временных защит, прямого управления пином реле
 * и автоматического отключения при превышении максимального времени работы.
 */

#include "heater_channel.h"

HeaterChannel::HeaterChannel(uint8_t relayPin)
    : _pin(relayPin)
    , _state(false)
    , _turnOnTime(0)
    , _turnOffTime(0)
    , _cooldownActive(false)
{}

void HeaterChannel::begin() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, LOW);
}

void HeaterChannel::update() {
    uint32_t now = millis();

    if (_state) {
        // Автоотключение при превышении лимита времени
        if (now - _turnOnTime >= static_cast<uint32_t>(HEATER_MAX_ON_TIME_S) * 1000U) {
            turnOff();
        }
    } else {
        // Снятие блокировки после паузы
        if (_cooldownActive &&
            now - _turnOffTime >= static_cast<uint32_t>(HEATER_MIN_PAUSE_S) * 1000U) {
            _cooldownActive = false;
        }
    }
}

bool HeaterChannel::turnOn() {
    if (_state) return true;          // уже включён
    if (_cooldownActive) return false; // ещё не прошла пауза

    setRelay(true);
    _state = true;
    _turnOnTime = millis();
    return true;
}

bool HeaterChannel::turnOff() {
    if (!_state) return true;         // уже выключен

    setRelay(false);
    _state = false;
    _turnOffTime = millis();
    _cooldownActive = true;           // старт паузы
    return true;
}

bool HeaterChannel::isOn() const {
    return _state;
}

bool HeaterChannel::canTurnOn() const {
    return !_state && !_cooldownActive;
}

uint32_t HeaterChannel::getTurnOffTime() const {
    return _turnOffTime;
}

void HeaterChannel::setRelay(bool on) {
    digitalWrite(_pin, on ? HIGH : LOW);
}