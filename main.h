#include "nrf5x-compat.h"
#include "ble_stack.h"

#ifndef RANDOM_ROTATE_KEYS
#define RANDOM_ROTATE_KEYS 1
#endif

#ifndef MAX_KEYS
// Maximum number of public keys to rotate
// Can be set during compilation with make MAX_KEYS=10
#define MAX_KEYS 50
#endif

#ifndef KEY_ROTATION_INTERVAL
// Key rotation interval in seconds
#define KEY_ROTATION_INTERVAL 3600 * 3
#endif