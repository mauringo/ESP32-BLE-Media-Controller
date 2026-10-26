#include "HidConnectionState.h"
#include <cassert>
#include <cstdint>

int main() {
  HidConnectionState state;
  assert(!state.mediaReady(0));
  state.onConnect(0);
  for (uint32_t now = 0; now < 1000; ++now) assert(!state.mediaReady(now));
  state.onAuthentication(true, 1000);
  assert(!state.mediaReady(3000)); // Encryption alone never enables input.
  state.onKeyboardSubscription(true, 1100);
  state.onMediaSubscription(true, 1200);
  assert(!state.mediaReady(1199)); // Caller sampled time before a callback ran.
  for (uint32_t now = 1200; now < 2200; ++now) assert(!state.mediaReady(now));
  assert(state.keyboardReady(2200) && state.mediaReady(2200));
  state.onMediaSubscription(false, 2300);
  assert(!state.mediaReady(10000));
  state.onDisconnect();
  assert(!state.keyboardReady(10000));
  state.onConnect(10000);
  state.onMediaSubscription(true, 10100); // Restore can precede encryption.
  assert(!state.mediaReady(20000));
  state.onAuthentication(true, 20100);
  assert(!state.mediaReady(21099));
  assert(state.mediaReady(21100));
  state.onAuthentication(true, 21200); // Re-encryption restarts quiet period.
  assert(!state.mediaReady(22199));
  assert(state.mediaReady(22200));
  state.onAuthentication(false, 22300);
  assert(!state.mediaReady(30000));
  state.onDisconnect();
  state.onConnect(31000);
  state.onAuthentication(true, 31000);
  assert(!state.mediaReady(40000)); // No stale subscription after reconnect.
  const uint32_t start = UINT32_MAX - 500;
  state.onConnect(start);
  state.onAuthentication(true, start);
  state.onMediaSubscription(true, start);
  assert(!state.mediaReady(uint32_t(start + 999)));
  assert(state.mediaReady(uint32_t(start + 1000)));
}
