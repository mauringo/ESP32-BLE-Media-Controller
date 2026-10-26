#include "PairingHold.h"
#include <cassert>
#include <cstdint>
int main() {
 PairingHold h;
 assert(!h.update(false, 0));
 assert(!h.update(true, 100));
 assert(!h.update(true, 12099));
 assert(h.update(true, 12100));
 assert(!h.update(true, 25000));
 assert(!h.update(false, 25001));
 assert(!h.update(true, 26000));
 assert(!h.update(false, 37999));
 assert(!h.update(true, 38000));
 assert(!h.update(true, 49999));
 assert(h.update(true, 50000));
 PairingHold rollover;
 const uint32_t start = UINT32_MAX - 6000;
 assert(!rollover.update(true, start));
 assert(!rollover.update(true, uint32_t(start + 11999)));
 assert(rollover.update(true, uint32_t(start + 12000)));
}
