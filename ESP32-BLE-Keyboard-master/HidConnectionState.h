#pragma once
#include <atomic>
#include <stdint.h>

// Callbacks can run on the NimBLE task while the application sends reports.
class HidConnectionState {
public:
  static constexpr uint32_t settleMs = 1000;
  void onConnect(uint32_t now) {
    secure = keyboard = media = false;
    changed = now;
    linked = true;
  }
  void onDisconnect() {
    linked = false;
    secure = keyboard = media = false;
  }
  void onAuthentication(bool encrypted, uint32_t now) { changed = now; secure = encrypted; }
  void onKeyboardSubscription(bool enabled, uint32_t now) { changed = now; keyboard = enabled; }
  void onMediaSubscription(bool enabled, uint32_t now) { changed = now; media = enabled; }
  bool connected() const { return linked.load(); }
  bool encrypted() const { return linked && secure; }
  bool keyboardSubscribed() const { return keyboard.load(); }
  bool mediaSubscribed() const { return media.load(); }
  bool keyboardReady(uint32_t now) const { return encrypted() && keyboard && stable(now); }
  bool mediaReady(uint32_t now) const { return encrypted() && media && stable(now); }
private:
  bool stable(uint32_t now) const {
    // A callback may publish a timestamp just after the caller sampled now.
    return int32_t(now - changed.load()) >= int32_t(settleMs);
  }
  std::atomic<uint32_t> changed{0};
  std::atomic<bool> linked{false}, secure{false}, keyboard{false}, media{false};
};
