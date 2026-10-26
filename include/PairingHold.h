#pragma once
#include <stdint.h>

// One trigger per continuous hold, including across millis() rollover.
class PairingHold {
public:
    bool update(bool pressed, uint32_t now) {
        if (!pressed) {
            holding = fired = false;
            return false;
        }
        if (!holding) {
            holding = true;
            started = now;
        }
        if (!fired && uint32_t(now - started) >= 12000) {
            fired = true;
            return true;
        }
        return false;
    }
    bool active() const { return holding; }
private:
    bool holding = false;
    bool fired = false;
    uint32_t started = 0;
};
