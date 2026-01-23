#ifndef MAGWICK_H
#define MAGWICK_H

#include <stdint.h>

typedef struct
{
    float q0;
    float q1;
    float q2;
    float q3;
    float beta;
    float sample_freq;
} madgwick_t;

void madgwick_init(madgwick_t *filt, float sample_freq, float beta);

void madgwick_update_imu(
    madgwick_t *filt,
    float gx, float gy, float gz, // rad/s
    float ax, float ay, float az  // m/s^2 (o normalizzato)
);

void madgwick_get_euler(
    const madgwick_t *filt,
    float *roll,
    float *pitch
);

#endif