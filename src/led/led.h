#ifndef LED_H
#define LED_H

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include "config.h"

#define LED_EVT_HEARTBEAT       BIT(0)
#define LED_EVT_CONNECTED       BIT(1)
#define LED_EVT_LISTENING       BIT(2)

typedef enum {
    LED_MODE_OFF = 0,
    LED_MODE_HEARTBEAT,
    LED_MODE_CONNECTED,
    LED_MODE_LISTENING,
} led_mode_t; 

void led_init(void);
void led_set_mode(led_mode_t mode);

#endif