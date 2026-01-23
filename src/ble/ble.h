#ifndef BLE_H
#define BLE_H

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>
#include <zephyr/bluetooth/uuid.h>
#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/conn.h>
#include <zephyr/bluetooth/gatt.h>
#include <zephyr/logging/log.h>

#include "config.h"
#include "adpcm/adpcm.h"
#include "imu/imu.h"

extern struct k_event app_events;

int ble_init(void);
int ble_adv_start(void);
int ble_audio_notify(const adpcm_packet_t *pkt);
int ble_imu_notify(const imu_packet_t *pkt);

#endif /* BLE_H */
