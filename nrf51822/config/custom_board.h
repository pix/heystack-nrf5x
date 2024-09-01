#pragma once

#include "nrf_gpio.h"
#include "nordic_common.h"

#ifndef STRINGIFY
#define STRINGIFY(s) XSTR(s)
#define XSTR(s) #s
#endif

#if defined (CUSTOM_BOARD_INC)
  #include STRINGIFY(CUSTOM_BOARD_INC.h)
#endif