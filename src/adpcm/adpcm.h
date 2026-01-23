#ifndef ADPCM_H
#define ADPCM_H

#include <stdint.h>
#include <stddef.h>

#include "config.h"

struct adpcm_state {
    int16_t prev_sample;
    int8_t  index;
};

/* ADPCM message queue */
struct adpcm_packet {
    uint8_t buf[PCM_BLOCK_SIZE / 4];
    size_t len;
};

typedef struct adpcm_packet adpcm_packet_t;

/* Init/reset state (da chiamare a inizio streaming) */
void adpcm_init(struct adpcm_state *st);

/*
 * Encode PCM16 → IMA ADPCM
 *
 * pcm        : buffer PCM signed 16-bit
 * samples    : numero di campioni PCM
 * out        : buffer output (samples / 2 bytes)
 * st         : stato persistente
 *
 * return     : numero di byte scritti in out
 */
size_t adpcm_encode(const int16_t *pcm,
                    size_t samples,
                    uint8_t *out,
                    struct adpcm_state *st);

#endif /* ADPCM_H */
