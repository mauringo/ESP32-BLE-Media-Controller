#include "RemoteControl.h"
#include "PairingHold.h"
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <cstring>

namespace {
bool pairingResetPending = false;
uint32_t pairingResetStarted = 0;
PairingHold pairingHold;
bool pairingHoldArmed = false;
// Factory calibration measured via USB on this board; optional NVS override.
int pairAdcMin = 2410;
int pairAdcMax = 2502;
int idleAdc = 0;
bool calibratingPair = false;
uint32_t calibrationStarted = 0;
uint32_t calibrationSum = 0;
unsigned calibrationSamples = 0;
int calibrationMin = 4095;
int calibrationMax = 0;

void printBluetoothStatus()
{
    Serial.printf("[BT] link=%d encrypted=%d HID=%d inputs=%d bonds=%d\n",
                  bleKeyboard.isLinkConnected(), bleKeyboard.isEncrypted(),
                  bleKeyboard.isConnected(), inputCommandsArmed, NimBLEDevice::getNumBonds());
    Serial.printf("[CONFIG] sleep=%lu ms ADC=%d pair=%d..%d\n",
                  static_cast<unsigned long>(AUTO_SLEEP_TIMEOUT_MS),
                  readButtonsAdc(), pairAdcMin, pairAdcMax);
}

void printBondStatus()
{
    const int bonds = NimBLEDevice::getNumBonds();
    Serial.printf("[BOND] stored=%d\n", bonds);
    for (int i = 0; i < bonds; ++i) {
        const auto address = NimBLEDevice::getBondedAddress(i).toString();
        Serial.printf("[BOND] #%d peer_id=%s\n", i, address.c_str());
    }
}

void handleSerialCommand(const char *command)
{
    if (command[0] != '\0') markActivity();
    if (strcmp(command, "help") == 0) {
        Serial.println("Comandi: status | pair | calibrate-pair (tenere 1+3) | restart | help");
    } else if (strcmp(command, "status") == 0) {
        printBluetoothStatus();
    } else if (strcmp(command, "calibrate-pair") == 0) {
        if (pairingResetPending) return;
        calibratingPair = true;
        calibrationStarted = millis();
        calibrationSum = calibrationSamples = 0;
        calibrationMin = 4095;
        calibrationMax = 0;
        Serial.println("[BT] Calibrazione: tenere SOLO 1+3 insieme per 2 secondi.");
    } else if (strcmp(command, "restart") == 0) {
        Serial.println("[BT] Riavvio scheda; abbinamenti conservati.");
        Serial.flush();
        delay(100);
        ESP.restart();
    } else if (strcmp(command, "pair") == 0) {
        if (pairingResetPending) {
            return;
        }
        Serial.println("[BT] Reset pairing esplicito: verranno rimossi solo i bond ESP32; eliminare anche RingosRemote_2 sul Mac.");
        printBondStatus();
        auto *server = NimBLEDevice::getServer();
        server->advertiseOnDisconnect(false);
        NimBLEDevice::getAdvertising()->stop();
        pairingResetPending = true;
        calibratingPair = false;
        pairingResetStarted = millis();
        for (auto handle : server->getPeerDevices()) {
            server->disconnect(handle);
        }
    } else if (command[0] != '\0') {
        Serial.println("Comando sconosciuto. Scrivi help e premi Invio.");
    }
}

void calibratePairLoop()
{
    markActivity();
    const int adc = readButtonsAdc();
    calibrationSum += adc;
    ++calibrationSamples;
    if (adc < calibrationMin) calibrationMin = adc;
    if (adc > calibrationMax) calibrationMax = adc;
    if (millis() - calibrationStarted < 2000) return;

    calibratingPair = false;
    const int center = calibrationSum / calibrationSamples;
    const int low = center - 40;
    const int high = center + 40;
    bool distinct = low > 0 && high < 4095 && abs(center - idleAdc) > 160;
    for (int i = 0; i < btnCount; ++i) {
        if (low <= voltageRanges[i][1] && high >= voltageRanges[i][0]) distinct = false;
    }
    if (!distinct || calibrationMax - calibrationMin > 60) {
        Serial.printf("[BT] Calibrazione rifiutata: ADC=%d min=%d max=%d idle=%d. Combinazione instabile o non distinguibile dai tasti singoli.\n",
                      center, calibrationMin, calibrationMax, idleAdc);
        return;
    }
    Preferences prefs;
    if (!prefs.begin("remote-bt", false)) {
        Serial.println("[BT] Impossibile salvare calibrazione.");
        return;
    }
    // Store both bounds in one NVS value, so an interrupted save is atomic.
    const uint32_t packed = (uint32_t(low) << 16) | uint32_t(high);
    const bool saved = prefs.putUInt("pair-adc", packed) == sizeof(packed);
    prefs.end();
    if (!saved) {
        Serial.println("[BT] Salvataggio calibrazione fallito.");
        return;
    }
    pairAdcMin = low;
    pairAdcMax = high;
    pairingHoldArmed = false;
    Serial.printf("[BT] Calibrazione salvata: ADC=%d..%d. Rilasciare i tasti. Poi 1+3 per 12 secondi = reset pairing.\n", low, high);
}

void pairingHoldLoop()
{
    const int adc = readButtonsAdc();
    const bool pressed = pairAdcMin >= 0 && adc >= pairAdcMin && adc <= pairAdcMax;
    // Require release after boot/calibration to avoid repeated resets if held.
    if (!pairingHoldArmed) {
        if (!pressed) pairingHoldArmed = true;
        return;
    }
    const bool wasHolding = pairingHold.active();
    if (pairingHold.update(pressed, millis())) {
        handleSerialCommand("pair");
    } else if (!wasHolding && pairingHold.active()) {
        Serial.println("[BT] 1+3 premuti: mantenerli per 12 secondi per cancellare gli abbinamenti.");
    } else if (wasHolding && !pairingHold.active()) {
        Serial.println("[BT] Tasti rilasciati: conto dei 12 secondi annullato.");
    }
}

void serialCommandLoop()
{
    static char command[32];
    static size_t length = 0;
    static bool overflow = false;
    // A bounded, non-blocking parser keeps encoder and BLE recovery responsive.
    for (size_t count = 0; count < 64 && Serial.available(); ++count) {
        const char c = Serial.read();
        if (c == '\r' || c == '\n') {
            command[length] = '\0';
            if (overflow) {
                Serial.println("Comando troppo lungo.");
            } else {
                handleSerialCommand(command);
            }
            length = 0;
            overflow = false;
        } else if (length < sizeof(command) - 1) {
            command[length++] = c;
        } else {
            overflow = true;
        }
    }
}

void pairingResetLoop()
{
    auto *server = NimBLEDevice::getServer();
    if (server->getConnectedCount() != 0) {
        if (millis() - pairingResetStarted >= 5000) {
            Serial.println("[BT] Disconnessione non completata: reset pairing annullato. Riprovare pair.");
            pairingResetPending = false;
            // Advertising is recovered by bleKeyboard.update(), as in normal operation.
        }
        return;
    }
    // No active host can save a new bond while the old keys are being removed.
    if (!NimBLEDevice::deleteAllBonds() || NimBLEDevice::getNumBonds() != 0) {
        Serial.println("[BT] Cancellazione abbinamenti fallita. Riprovare pair.");
        printBondStatus();
        pairingResetPending = false;
        // Advertising is recovered by bleKeyboard.update(), as in normal operation.
        return;
    }
    Serial.println("[BT] Bond ESP32 cancellati e verificati (0 rimasti). Riavvio.");
    Serial.println("[BT] Sul Mac dimentica il vecchio RingosRemote_2, poi premi Connetti.");
    Serial.flush();
    delay(100);
    ESP.restart();
}

} // namespace

void beginPairingControls()
{
    idleAdc = readButtonsAdc();
    Preferences prefs;
    if (prefs.begin("remote-bt", false)) {
        const uint32_t packed = prefs.getUInt("pair-adc", 0);
        prefs.end();
        if (packed != 0) {
            pairAdcMin = packed >> 16;
            pairAdcMax = packed & 0xffff;
        }
    }
}

bool pairingControlsBusy()
{
    return pairingResetPending || calibratingPair || pairingHold.active();
}

bool pollPairingControls()
{
    serialCommandLoop();
    if (pairingResetPending) {
        pairingResetLoop();
        delay(10);
        return true;
    }
    if (calibratingPair) {
        calibratePairLoop();
        delay(50);
        return true;
    }
    pairingHoldLoop();
    if (pairingResetPending || pairingHold.active()) {
        delay(10);
        return true;
    }
    return false;
}
