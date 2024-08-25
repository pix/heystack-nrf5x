#ifndef _NRF5X_COMPAT_H_
#define _NRF5X_COMPAT_H_

#include "nordic_common.h"
#include "nrf.h"

#if NRF_SD_BLE_API_VERSION <= 3
#define NRF_SDK_VERSION 12
#else
#define NRF_SDK_VERSION 15
#endif

#include "nrf_log.h"
#include "nrf_log_ctrl.h"

#if NRF_SDK_VERSION >= 15
#include "nrf_log_default_backends.h"
#define COMPAT_APP_TIMER_TICKS(APP_TIMER_MS) APP_TIMER_TICKS(APP_TIMER_MS)
#define COMPAT_NRF_LOG_INFO(...) NRF_LOG_INFO(__VA_ARGS__)
#else
#define COMPAT_APP_TIMER_TICKS(APP_TIMER_MS) APP_TIMER_TICKS(APP_TIMER_MS, APP_TIMER_PRESCALER)
#define COMPAT_NRF_LOG_INFO(arg, ...) NRF_LOG_INFO(arg "\n", ##__VA_ARGS__)
#endif

#endif
