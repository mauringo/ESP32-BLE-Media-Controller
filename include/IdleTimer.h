#pragma once
#include <stdint.h>

class IdleTimer {
public:
    void activity(uint32_t now) { last = now; }
    uint32_t elapsed(uint32_t now) const { return uint32_t(now - last); }
    bool expired(uint32_t now, uint32_t timeout) const {
        return timeout != 0 && elapsed(now) >= timeout;
    }
private:
    uint32_t last = 0;
};
