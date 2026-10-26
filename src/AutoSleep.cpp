#include "RemoteControl.h"
#include "StatusLed.h"
#include <NimBLEDevice.h>
#include <esp_sleep.h>
#include <driver/gpio.h>

namespace {
void enterAutoSleep()
{
    // Do not sleep during a hold (including the 12-second pairing gesture).
    if (anyButtonHeld() || pairingControlsBusy()) return;
    const uint8_t pinsBefore = encoderPinState();
    const long positionBefore = rotaryEncoder.readEncoder();
    Serial.println("[SLEEP] Inattivita: Bluetooth spento. Pulsante o encoder per riattivare.");
    bleKeyboard.releaseAll();
    auto *server = NimBLEDevice::getServer();
    server->advertiseOnDisconnect(false);
    NimBLEDevice::getAdvertising()->stop();
    // Keep C++ objects: the server callbacks refer to the global bleKeyboard.
    // Wake uses a clean restart; bonds and button calibration stay in NVS.
    if (!NimBLEDevice::deinit(false)) {
        Serial.println("[SLEEP] Arresto Bluetooth fallito; riavvio di recupero.");
        Serial.flush();
        ESP.restart();
    }
    detachInterrupt(ROTARY_ENCODER_A_PIN);
    detachInterrupt(ROTARY_ENCODER_B_PIN);

    const gpio_num_t wakePins[] = {
        gpio_num_t(ROTARY_ENCODER_A_PIN), gpio_num_t(ROTARY_ENCODER_B_PIN),
        gpio_num_t(ROTARY_ENCODER_BUTTON_PIN)
    };
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    for (auto pin : wakePins) {
        const int idle = gpio_get_level(pin);
        if (gpio_wakeup_enable(pin, idle ? GPIO_INTR_LOW_LEVEL : GPIO_INTR_HIGH_LEVEL) != ESP_OK) {
            Serial.println("[SLEEP] Configurazione GPIO fallita; riavvio.");
            Serial.flush();
            ESP.restart();
        }
    }
    if (esp_sleep_enable_gpio_wakeup() != ESP_OK ||
        esp_sleep_enable_timer_wakeup(uint64_t(AUTO_SLEEP_ADC_POLL_MS) * 1000) != ESP_OK) {
        Serial.println("[SLEEP] Configurazione wakeup fallita; riavvio.");
        Serial.flush();
        ESP.restart();
    }
    Serial.flush();
    // Activity arriving while Bluetooth shuts down must wake immediately too.
    if (encoderPinState() != pinsBefore || rotaryEncoder.readEncoder() != positionBefore || anyButtonHeld()) {
        ESP.restart();
    }
    turnOffStatusLed();
    while (true) {
        const esp_err_t result = esp_light_sleep_start();
        const auto cause = esp_sleep_get_wakeup_cause();
        if (result != ESP_OK || cause == ESP_SLEEP_WAKEUP_GPIO ||
            encoderPinState() != pinsBefore || anyButtonHeld()) {
            // The wake gesture is used to wake, not sent as a media command.
            ESP.restart();
        }
        // Timer wake: sample the resistor ladder and return to light sleep.
    }
}

} // namespace

void pollAutoSleep()
{
    if (inactivity.expired(millis(), AUTO_SLEEP_TIMEOUT_MS)) enterAutoSleep();
}
