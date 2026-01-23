#include "fusion/madgwick.h"
#include <math.h>

#define INV_SQRT(x) (1.0f / sqrtf(x))

void madgwick_init(madgwick_t *f, float sample_freq, float beta)
{
    f->q0 = 1.0f;
    f->q1 = 0.0f;
    f->q2 = 0.0f;
    f->q3 = 0.0f;
    f->sample_freq = sample_freq;
    f->beta = beta;
}

void madgwick_update_imu(
    madgwick_t *f,
    float gx, float gy, float gz,
    float ax, float ay, float az
)
{
    float q0 = f->q0, q1 = f->q1, q2 = f->q2, q3 = f->q3;
    float recip_norm;
    float s0, s1, s2, s3;
    float q_dot0, q_dot1, q_dot2, q_dot3;
    float _2q0 = 2.0f * q0;
    float _2q1 = 2.0f * q1;
    float _2q2 = 2.0f * q2;
    float _2q3 = 2.0f * q3;
    float _4q0 = 4.0f * q0;
    float _4q1 = 4.0f * q1;
    float _4q2 = 4.0f * q2;
    float _8q1 = 8.0f * q1;
    float _8q2 = 8.0f * q2;
    float q0q0 = q0 * q0;
    float q1q1 = q1 * q1;
    float q2q2 = q2 * q2;
    float q3q3 = q3 * q3;

    if (ax == 0.0f && ay == 0.0f && az == 0.0f)
        return;

    recip_norm = INV_SQRT(ax * ax + ay * ay + az * az);
    ax *= recip_norm;
    ay *= recip_norm;
    az *= recip_norm;

    s0 = _4q0 * q2q2 + _2q2 * ax + _4q0 * q1q1 - _2q1 * ay;
    s1 = _4q1 * q3q3 - _2q3 * ax + 4.0f * q0q0 * q1 - _2q0 * ay - _4q1 + _8q1 * q1q1 + _8q1 * q2q2 + _4q1 * az;
    s2 = 4.0f * q0q0 * q2 + _2q0 * ax + _4q2 * q3q3 - _2q3 * ay - _4q2 + _8q2 * q1q1 + _8q2 * q2q2 + _4q2 * az;
    s3 = 4.0f * q1q1 * q3 - _2q1 * ax + 4.0f * q2q2 * q3 - _2q2 * ay;

    recip_norm = INV_SQRT(s0*s0 + s1*s1 + s2*s2 + s3*s3);
    s0 *= recip_norm;
    s1 *= recip_norm;
    s2 *= recip_norm;
    s3 *= recip_norm;

    q_dot0 = 0.5f * (-q1 * gx - q2 * gy - q3 * gz) - f->beta * s0;
    q_dot1 = 0.5f * ( q0 * gx + q2 * gz - q3 * gy) - f->beta * s1;
    q_dot2 = 0.5f * ( q0 * gy - q1 * gz + q3 * gx) - f->beta * s2;
    q_dot3 = 0.5f * ( q0 * gz + q1 * gy - q2 * gx) - f->beta * s3;

    q0 += q_dot0 / f->sample_freq;
    q1 += q_dot1 / f->sample_freq;
    q2 += q_dot2 / f->sample_freq;
    q3 += q_dot3 / f->sample_freq;

    recip_norm = INV_SQRT(q0*q0 + q1*q1 + q2*q2 + q3*q3);
    f->q0 = q0 * recip_norm;
    f->q1 = q1 * recip_norm;
    f->q2 = q2 * recip_norm;
    f->q3 = q3 * recip_norm;
}

void madgwick_get_euler(
    const madgwick_t *f,
    float *roll,
    float *pitch
)
{
    *roll  = atan2f(2.0f*(f->q0*f->q1 + f->q2*f->q3),
                    1.0f - 2.0f*(f->q1*f->q1 + f->q2*f->q2));

    *pitch = asinf(2.0f*(f->q0*f->q2 - f->q3*f->q1));
}
