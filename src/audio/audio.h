#ifndef AUDIO_H
#define AUDIO_H

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/audio/dmic.h>
#include <zephyr/audio/codec.h>

#include "config.h"
#include "adpcm/adpcm.h"

/* Thread */
#define AUDIO_STACK_SIZE    2048

/* DeviceTree */
#define DMIC_NODE DT_INST(0, nordic_nrf_pdm)

// K_MSGQ_EXTERN(audio_queue);
extern struct k_msgq audio_queue;

/* API */
void audio_thread_init(void);
int audio_init(void);

extern struct k_event app_events;


#endif /* AUDIO_H */
