#ifndef CJ96_DEVICE_DISCOVERY_TIMING_H
#define CJ96_DEVICE_DISCOVERY_TIMING_H

// Shared by the sequential scanner and Window2's estimated countdown. Durations are ms.
namespace cj96_discovery {
static const int ADDRESS_MIN = 20;
static const int ADDRESS_MAX = 255;
static const int ADDRESS_COUNT = ADDRESS_MAX - ADDRESS_MIN + 1;
static const int PROBE_WAIT_MS = 250;
static const int RETRY_WAIT_MS = 350;
// One full address-by-address pass plus one retry pass for missed addresses.
static const int RETRY_PASSES = 1;
// Seven-byte request, 2400 baud, 8N1; include drain/scheduling allowance.
static const int REQUEST_ALLOWANCE_MS = 40;
// Keep the scanner timing unchanged. This value is retained for compatibility
// with the UI, which no longer presents it as a countdown.
static const int ESTIMATED_MAX_MS = ADDRESS_COUNT *
    (PROBE_WAIT_MS + REQUEST_ALLOWANCE_MS +
     RETRY_PASSES * (RETRY_WAIT_MS + REQUEST_ALLOWANCE_MS));
}
#endif
