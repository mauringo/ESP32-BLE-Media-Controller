#include "IdleTimer.h"
#include <cassert>
#include <cstdint>

int main() {
    IdleTimer timer;
    timer.activity(100);
    assert(!timer.expired(10099, 10000));
    assert(timer.expired(10100, 10000));
    assert(!timer.expired(10100, 0));
    timer.activity(10100); // Activity, including a held button, restarts timeout.
    assert(!timer.expired(20099, 10000));
    assert(timer.expired(20100, 10000));
    const uint32_t start = UINT32_MAX - 5000;
    timer.activity(start);
    assert(!timer.expired(uint32_t(start + 9999), 10000));
    assert(timer.expired(uint32_t(start + 10000), 10000));
}
