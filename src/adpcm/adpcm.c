#include "adpcm/adpcm.h"

/* Step size table */
static const int step_table[89] = {
     7,  8,  9, 10, 11, 12, 13, 14,
    16, 17, 19, 21, 23, 25, 28, 31,
    34, 37, 41, 45, 50, 55, 60, 66,
    73, 80, 88, 97,107,118,130,143,
   157,173,190,209,230,253,279,307,
   337,371,408,449,494,544,598,658,
   724,796,876,963,1060,1166,1282,1411,
  1552,1707,1878,2066,2272,2499,2749,3024,
  3327,3660,4026,4428,4871,5358,5894,6484,
  7132,7845,8630,9493,10442,11487,12635,13899,
  15289,16818,18500,20350,22385,24623,27086,29794,
  32767
};

/* Index adjust table */
static const int index_table[16] = {
   -1, -1, -1, -1,
    2,  4,  6,  8,
   -1, -1, -1, -1,
    2,  4,  6,  8
};

void adpcm_init(struct adpcm_state *st)
{
    st->prev_sample = 0;
    st->index = 0;
}

size_t adpcm_encode(const int16_t *pcm,
                    size_t samples,
                    uint8_t *out,
                    struct adpcm_state *st)
{
    int predictor = st->prev_sample;
    int index = st->index;
    int step = step_table[index];

    uint8_t out_byte = 0;
    int nibble = 0;
    size_t out_idx = 0;

    for (size_t i = 0; i < samples; i++) {

        int diff = pcm[i] - predictor;
        int sign = (diff < 0);
        if (sign) diff = -diff;

        int delta = 0;
        int temp = step;

        if (diff >= temp) { delta |= 4; diff -= temp; }
        temp >>= 1;
        if (diff >= temp) { delta |= 2; diff -= temp; }
        temp >>= 1;
        if (diff >= temp) { delta |= 1; }

        delta |= sign << 3;

        /* Update predictor */
        int diffq = step >> 3;
        if (delta & 4) diffq += step;
        if (delta & 2) diffq += step >> 1;
        if (delta & 1) diffq += step >> 2;

        predictor += sign ? -diffq : diffq;

        if (predictor > 32767) predictor = 32767;
        if (predictor < -32768) predictor = -32768;

        /* Update index */
        index += index_table[delta & 0x0F];
        if (index < 0) index = 0;
        if (index > 88) index = 88;

        step = step_table[index];

        /* Pack nibbles */
        if (nibble == 0) {
            out_byte = delta & 0x0F;
            nibble = 1;
        } else {
            out[out_idx++] = out_byte | ((delta & 0x0F) << 4);
            nibble = 0;
        }
    }

    if (nibble) {
        out[out_idx++] = out_byte;
    }

    st->prev_sample = predictor;
    st->index = index;

    return out_idx;
}
