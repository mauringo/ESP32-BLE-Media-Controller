#pragma once
#include <Arduino.h>
#include <AiEsp32RotaryEncoder.h>
#include <BleKeyboard.h>
#include <MultiButtons.h>
#include "IdleTimer.h"
#include "ProjectConfig.h"

constexpr int ROTARY_ENCODER_A_PIN = 12;
constexpr int ROTARY_ENCODER_B_PIN = 11;
constexpr int ROTARY_ENCODER_BUTTON_PIN = 13;
constexpr int ROTARY_ENCODER_VCC_PIN = -1;
constexpr int ROTARY_ENCODER_STEPS = 4;
constexpr int btnPin = 8;

extern BleKeyboard bleKeyboard;
extern AiEsp32RotaryEncoder rotaryEncoder;
extern MultiButtons mb;
extern IdleTimer inactivity;
extern bool inputCommandsArmed;
extern int voltageRanges[3][2];
constexpr int btnCount = 3;

void markActivity();
int readButtonsAdc();
uint8_t encoderPinState();
bool anyButtonHeld();
void beginRemoteInputs();
void pollRemoteInputs();
void trackInputActivity();
void updateInputArming();
void beginPairingControls();
bool pollPairingControls();
bool pairingControlsBusy();
void pollAutoSleep();
