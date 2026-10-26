#include "RemoteControl.h"
#include "StatusLed.h"

void setup()
{
    Serial.begin(115200);
    beginStatusLed();
    analogReadResolution(12);
    beginPairingControls();
    beginRemoteInputs();
    bleKeyboard.begin();
    markActivity();
    Serial.printf("RingosRemote_2 pronto. Autosleep: %lu ms.\n",
                  static_cast<unsigned long>(AUTO_SLEEP_TIMEOUT_MS));
    Serial.println("Comandi USB: help, status, pair, calibrate-pair, restart.");
}

void loop()
{
    trackInputActivity();
    updateStatusLed();
    if (pollPairingControls()) return;
    bleKeyboard.update();
    updateInputArming();
    pollRemoteInputs();
    pollAutoSleep();
    delay(50);
}
