#include "sleep/sleep.h"

LOG_MODULE_REGISTER(sleep);

static struct k_work_delayable sleep_work;


void reset_sleep(void)
{
    k_work_reschedule(&sleep_work, INACTIVITY_TIMEOUT);
}

void enter_sleep(struct k_work *work)
{
    LOG_INF("Entering System OFF");

    // Spegni BLE
    bt_disable();

    // Deep sleep: reset MCU, wake on LSM6DS3
    sys_poweroff();
}

void inactivity_init(void)
{
    k_work_init_delayable(&sleep_work, enter_sleep);
    k_work_schedule(&sleep_work, INACTIVITY_TIMEOUT);
}