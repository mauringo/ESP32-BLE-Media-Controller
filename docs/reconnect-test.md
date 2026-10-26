# Reconnection test summary

This report summarizes a firmware validation run on 25 October 2026. Raw logs,
snapshots, and peer identifiers are intentionally excluded from this public
repository.

## Results

- 10 of 10 software restarts reached a stable HID-ready state.
- Time to first complete HID confirmation ranged from 4.61 to 12.89 seconds;
  the median was 8.78 seconds.
- No panic or HID send error was observed in this series.
- The test covered software restarts, not repeated power removal or cold boots.
- Timings include reset, USB monitor recovery, and periodic state sampling;
  they are not precise BLE handshake measurements.

The host was reported to respond to media controls after reconnection. This
manual observation is not a substitute for automated end-to-end testing.

## Test conditions and limits

Automatic sleep and the startup media-key test were disabled for the run.
Another, unpaired peer attempted connections before the paired host in some
cycles. The reason for earlier reconnect delays was not established, and this
series does not identify or rule out the cause of previously observed panics.

Do not use this small software-restart sample as a reliability or battery-life
guarantee.
