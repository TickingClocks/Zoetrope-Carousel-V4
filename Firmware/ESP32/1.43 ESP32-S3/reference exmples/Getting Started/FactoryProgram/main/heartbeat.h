#pragma once

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Change this to your LED pin */
#define HEARTBEAT_GPIO GPIO_NUM_45

/* Public API */
void heartbeat_init(void);
void heartbeat_start(void);
void heartbeat_stop(void);

#ifdef __cplusplus
}
#endif
