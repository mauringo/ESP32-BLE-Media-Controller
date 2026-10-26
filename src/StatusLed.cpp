#include "StatusLed.h"
#include "RemoteControl.h"
#include <esp32-hal-rgb-led.h>
#include <NimBLEDevice.h>

namespace {
constexpr uint8_t statusLedPin = 48;
constexpr uint32_t blinkIntervalMs = 500;

enum class LedState : uint8_t {
    Disconnected,
    Discoverable,
    Connecting,
    Connected
};

LedState currentState = LedState::Disconnected;
bool ledOn = true;
uint32_t lastBlink = 0;

void writeLed(bool on)
{
    if (on) {
        switch (currentState) {
            case LedState::Disconnected: rgbLedWrite(statusLedPin, 32, 0, 0); break;
            case LedState::Discoverable: rgbLedWrite(statusLedPin, 0, 32, 0); break;
            case LedState::Connecting: rgbLedWrite(statusLedPin, 32, 16, 0); break;
            case LedState::Connected: rgbLedWrite(statusLedPin, 0, 32, 0); break;
        }
    } else {
        rgbLedWrite(statusLedPin, 0, 0, 0);
    }
}

void setState(LedState next, uint32_t now)
{
    if (currentState == next) return;
    currentState = next;
    ledOn = true;
    lastBlink = now;
    writeLed(ledOn);
}
} // namespace

void beginStatusLed()
{
    currentState = LedState::Disconnected;
    ledOn = true;
    writeLed(true);
}

void updateStatusLed()
{
    const uint32_t now = millis();
    if (bleKeyboard.isConnected()) {
        setState(LedState::Connected, now);
        return;
    }
    if (bleKeyboard.isLinkConnected()) {
        setState(LedState::Connecting, now);
        return;
    }
    if (NimBLEDevice::getAdvertising()->isAdvertising()) {
        setState(LedState::Discoverable, now);
        if (now - lastBlink >= blinkIntervalMs) {
            lastBlink = now;
            ledOn = !ledOn;
            writeLed(ledOn);
        }
        return;
    }
    setState(LedState::Disconnected, now);
}

void turnOffStatusLed()
{
    rgbLedWrite(statusLedPin, 0, 0, 0);
}
