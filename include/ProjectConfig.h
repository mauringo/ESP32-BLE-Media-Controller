#pragma once

// Project settings. Timeout 0 disables automatic sleep.
#ifndef AUTO_SLEEP_TIMEOUT_MS
// Three minutes balances battery saving with the Mac's reconnection delay.
#define AUTO_SLEEP_TIMEOUT_MS 180000
#endif
#ifndef AUTO_SLEEP_ADC_POLL_MS
// Fewer timer wakeups; analog buttons are still detected within about 50 ms.
#define AUTO_SLEEP_ADC_POLL_MS 50
#endif
#ifndef AUTO_SLEEP_ADC_ACTIVE_MIN
#define AUTO_SLEEP_ADC_ACTIVE_MIN 100
#endif
static_assert(AUTO_SLEEP_TIMEOUT_MS >= 0 && AUTO_SLEEP_TIMEOUT_MS <= 0x7fffffff,
              "Sleep timeout must be 0..2147483647 ms");
static_assert(AUTO_SLEEP_ADC_POLL_MS >= 10 && AUTO_SLEEP_ADC_POLL_MS <= 100,
              "ADC sleep polling must be 10..100 ms");
