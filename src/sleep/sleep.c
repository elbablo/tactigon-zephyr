#include "sleep/sleep.h"
#include "imu/imu.h"

LOG_MODULE_REGISTER(sleep);

static struct k_work_delayable sleep_work;


void reset_sleep(void)
{
    k_work_reschedule(&sleep_work, INACTIVITY_TIMEOUT);
}

void inactivity_stop(void)
{
    k_work_cancel_delayable(&sleep_work);
}

void enter_sleep(struct k_work *work)
{
    LOG_INF("Entering System OFF");

    // Arma wake-up su INT1 dell'IMU; se l'IMU si muove rimanda lo sleep
    if (imu_arm_wakeup() != 0)
    {
        reset_sleep();
        return;
    }

    // Spegni BLE
    bt_disable();

    // Deep sleep: reset MCU, wake su INT1 (LSM6DS3)
    sys_poweroff();
}

void inactivity_init(void)
{
    k_work_init_delayable(&sleep_work, enter_sleep);
    k_work_schedule(&sleep_work, INACTIVITY_TIMEOUT);
}