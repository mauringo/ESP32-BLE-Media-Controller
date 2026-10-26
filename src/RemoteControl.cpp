#include "RemoteControl.h"

BleKeyboard bleKeyboard("RingosRemote_2", "SuperMauro", 100);
IdleTimer inactivity;
bool inputCommandsArmed = false;
// Measured: button 3 = 2302-2317 ADC; buttons 1+3 = 2450-2462 ADC.
int voltageRanges[3][2] = {{600, 810}, {1400, 1620}, {2200, 2380}};
AiEsp32RotaryEncoder rotaryEncoder(ROTARY_ENCODER_A_PIN, ROTARY_ENCODER_B_PIN,
    ROTARY_ENCODER_BUTTON_PIN, ROTARY_ENCODER_VCC_PIN, ROTARY_ENCODER_STEPS);

namespace {
long previousEncoder = 0;

void sendMediaKey(const MediaKeyReport key)
{
    // Never replay gestures made before the HID session is stable.
    if (inputCommandsArmed && bleKeyboard.isConnected()) bleKeyboard.write(key);
}

void buttonHandler(MultiButtons *, int index)
{
    markActivity();
    switch (index) {
        case 0: sendMediaKey(KEY_MEDIA_PREVIOUS_TRACK); break;
        case 1: sendMediaKey(KEY_MEDIA_PLAY_PAUSE); break;
        case 2: sendMediaKey(KEY_MEDIA_NEXT_TRACK); break;
    }
}

void IRAM_ATTR readEncoderISR() { rotaryEncoder.readEncoder_ISR(); }
} // namespace

MultiButtons mb(btnPin, btnCount, voltageRanges, buttonHandler, 4095, BTN_TRIGGER_EDGE_PRESS);

void markActivity() { inactivity.activity(millis()); }

int readButtonsAdc()
{
    int sum = 0;
    for (int i = 0; i < 4; ++i) sum += analogRead(btnPin);
    return sum / 4;
}

uint8_t encoderPinState()
{
    return (digitalRead(ROTARY_ENCODER_A_PIN) ? 1 : 0)
         | (digitalRead(ROTARY_ENCODER_B_PIN) ? 2 : 0);
}

bool anyButtonHeld()
{
    return readButtonsAdc() > AUTO_SLEEP_ADC_ACTIVE_MIN
        || digitalRead(ROTARY_ENCODER_BUTTON_PIN) == LOW;
}

void beginRemoteInputs()
{
    rotaryEncoder.begin();
    rotaryEncoder.setup(readEncoderISR);
    rotaryEncoder.setBoundaries(-1000, 1000, false);
    rotaryEncoder.setAcceleration(0);
    previousEncoder = rotaryEncoder.readEncoder();
    mb.begin();
}

void pollRemoteInputs()
{
    if (rotaryEncoder.encoderChanged()) {
        markActivity();
        const long position = rotaryEncoder.readEncoder();
        if (position > previousEncoder) sendMediaKey(KEY_MEDIA_VOLUME_UP);
        if (position < previousEncoder) sendMediaKey(KEY_MEDIA_VOLUME_DOWN);
        previousEncoder = position;
    }
    if (rotaryEncoder.isEncoderButtonClicked()) {
        markActivity();
        static uint32_t lastClick = 0;
        if (millis() - lastClick >= 500) {
            lastClick = millis();
            sendMediaKey(KEY_MEDIA_MUTE);
        }
    }
    mb.loop();
}

void trackInputActivity()
{
    static uint8_t previousPins = encoderPinState();
    const uint8_t pins = encoderPinState();
    if (pins != previousPins || anyButtonHeld()) markActivity();
    previousPins = pins;
}

void updateInputArming()
{
    static bool previouslyReady = false;
    const bool ready = bleKeyboard.isConnected();
    if (ready && !previouslyReady) markActivity();
    previouslyReady = ready;
    static long previousPosition = rotaryEncoder.readEncoder();
    static uint8_t previousPins = encoderPinState();
    static bool quiet = false;
    static uint32_t quietSince = 0;
    const long position = rotaryEncoder.readEncoder();
    const uint8_t pins = encoderPinState();
    const bool active = anyButtonHeld() || position != previousPosition || pins != previousPins;
    previousPosition = position;
    previousPins = pins;
    if (!bleKeyboard.isConnected()) {
        inputCommandsArmed = false;
        quiet = false;
        return;
    }
    if (inputCommandsArmed) return;
    // A gesture that started before readiness cannot become a key press halfway
    // through reconnect. Require released buttons and a stationary encoder.
    if (active) {
        quiet = false;
    } else if (!quiet) {
        quiet = true;
        quietSince = millis();
    } else if (millis() - quietSince >= 300) {
        inputCommandsArmed = true;
        Serial.println("[HID] Ingressi abilitati dopo stabilizzazione e rilascio.");
    }
}

