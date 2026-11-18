#ifndef SLEEP_H
#define SLEEP_H

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/sys/poweroff.h>

#include "config.h"

void reset_sleep(void);
void inactivity_init(void);

#endif