#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/audio/dmic.h>

#include "ble/ble.h"
#include "audio/audio.h"
#include "adpcm/adpcm.h"
#include "imu/imu.h"

LOG_MODULE_REGISTER(main);

struct k_event app_events;

K_THREAD_STACK_DEFINE(ble_stack, 1024);
static struct k_thread ble_thread_data;

K_MSGQ_DEFINE(audio_queue, sizeof(adpcm_packet_t), 32, 4);
K_MSGQ_DEFINE(imu_queue, sizeof(imu_packet_t), 16, 4);

/* Thread entry: legge dalla coda ADPCM e invia BLE notify */
static void ble_thread(void *a, void *b, void *c)
{
    adpcm_packet_t audio_pkt;
    imu_packet_t imu_pkt;

    while (1)
    {
        if (k_msgq_get(&audio_queue, &audio_pkt, K_NO_WAIT) == 0)
        {
            ble_audio_notify(&audio_pkt);
        }

        if (k_msgq_get(&imu_queue, &imu_pkt, K_NO_WAIT) == 0)
        {
            ble_imu_notify(&imu_pkt);
        }

        k_sleep(K_MSEC(1));
    }
}


int main(void)
{
    k_event_init(&app_events);

    uint32_t events = 0;
    int err;

    err = ble_init();

    if (err)
    {
        LOG_ERR("Failed to initialize BLE %d", err);
        return err;
    }

    LOG_INF("BLE initialized!");

    err = audio_init();
    if (err)
    {
        LOG_ERR("Audio init failed");
    }

    err = imu_init();
    if (err)
    {
        LOG_ERR("IMU init failed");
    }

    k_thread_create(&ble_thread_data,
                ble_stack,
                K_THREAD_STACK_SIZEOF(ble_stack),
                ble_thread,
                NULL, NULL, NULL,
                BLE_THREAD_PRIORITY,
                0,
                K_NO_WAIT);

    while (1)
    {

        events = k_event_wait_safe(
            &app_events,
            EVT_BT_ALL,
            false,
            K_FOREVER);

        if (events & EVT_BT_CONNECTED)
        {
            LOG_INF("CONNECTED");
        }

        if (events & EVT_BT_DISCONNECTED)
        {
            LOG_INF("DISCONNECTED");
            ble_adv_start();
        }

        if (events & EVT_BT_SUB_CHAR_IMU)
        {
            LOG_INF("SUB IMU");
            imu_start();
        }

        if (events & EVT_BT_SUB_CHAR_AUDIO)
        {
            LOG_INF("SUB AUDIO");
            k_event_set(&app_events, EVT_AUDIO_START);
        }

        if (events & EVT_BT_UNSUB_CHAR_IMU)
        {
            LOG_INF("UNSUB IMU");
            imu_stop();
        }

        if (events & EVT_BT_UNSUB_CHAR_AUDIO)
        {
            LOG_INF("UNSUB AUDIO");
            k_event_clear(&app_events, EVT_AUDIO_START);
        }

        k_event_clear(&app_events, events);
    }

    return 0;
}
