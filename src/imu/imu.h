#ifndef IMU_H
#define IMU_H

#include <zephyr/logging/log.h>
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/drivers/sensor.h>

#include "config.h"
#include "fusion/madgwick.h"

#define IMU_STACK_SIZE      2048
#define IMU_PRIORITY        4

#define RAD_TO_DEG          (180.0f / (float)M_PI)
#define DEG_TO_RAD          0.01745329252f

struct imu_state {
    float ax, ay, az;
    float gx, gy, gz;
    float roll, pitch;
};

typedef struct imu_state imu_state_t;

struct imu_packet {
    uint8_t buff[BLE_BUFFER_LEN];
    size_t len;
};

typedef struct imu_packet imu_packet_t;

// void imu_thread_init(void);
int imu_init(void);
void imu_start(void);
void imu_stop(void);
// void imu_state_cb(struct k_timer *timer_id);
void imu_ble_cb(struct k_timer *timer_id);

extern struct k_event app_events;
extern struct k_msgq imu_queue;


#endif