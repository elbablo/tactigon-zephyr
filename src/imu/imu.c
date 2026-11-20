#include <zephyr/drivers/i2c.h>
#include <zephyr/drivers/gpio.h>
#include <soc.h>
#include <hal/nrf_gpio.h>

#include "imu/imu.h"

LOG_MODULE_REGISTER(imu);

#define IMU_NODE DT_COMPAT_GET_ANY_STATUS_OKAY(st_lsm6dsl)

/* Registri LSM6DSL non esposti dal driver Zephyr */
#define LSM6DSL_INT1_CTRL     0x0D
#define LSM6DSL_CTRL1_XL      0x10
#define LSM6DSL_WAKE_UP_SRC   0x1B
#define LSM6DSL_TAP_CFG       0x58
#define LSM6DSL_WAKE_UP_THS   0x5B
#define LSM6DSL_WAKE_UP_DUR   0x5C
#define LSM6DSL_MD1_CFG       0x5E

static const struct device *imu_dev = DEVICE_DT_GET_ONE(st_lsm6dsl);
static const struct i2c_dt_spec imu_i2c = I2C_DT_SPEC_GET(IMU_NODE);
static const struct gpio_dt_spec imu_int1 = GPIO_DT_SPEC_GET(IMU_NODE, irq_gpios);
static madgwick_t fusion;
struct k_timer imu_ble_timer;
struct imu_state imu_state;
struct imu_packet imu_pkt;
struct k_mutex imu_mutex;

struct sensor_value odr = {.val1 = 0, .val2 = 0};

struct sensor_trigger trig_acc = {
    .type = SENSOR_TRIG_DATA_READY,
    .chan = SENSOR_CHAN_ACCEL_XYZ};

struct sensor_trigger trig_gyro = {
    .type = SENSOR_TRIG_DATA_READY,
    .chan = SENSOR_CHAN_GYRO_XYZ};

void imu_data_ready_cb(const struct device *dev, const struct sensor_trigger *trig)
{
    struct sensor_value acc[3], gyr[3];

    // Legge entrambi i sensori
    sensor_sample_fetch_chan(dev, SENSOR_CHAN_ACCEL_XYZ);
    sensor_channel_get(dev, SENSOR_CHAN_ACCEL_XYZ, acc);

    sensor_sample_fetch_chan(dev, SENSOR_CHAN_GYRO_XYZ);
    sensor_channel_get(dev, SENSOR_CHAN_GYRO_XYZ, gyr);

    k_mutex_lock(&imu_mutex, K_FOREVER);
    imu_state.ax = (float)sensor_value_to_double(&acc[0]) / G_VALUE;
    imu_state.ay = (float)sensor_value_to_double(&acc[1]) / G_VALUE;
    imu_state.az = (float)sensor_value_to_double(&acc[2]) / G_VALUE;

    imu_state.gx = (float)sensor_value_to_double(&gyr[0]) * DEG_TO_RAD;
    imu_state.gy = (float)sensor_value_to_double(&gyr[1]) * DEG_TO_RAD;
    imu_state.gz = (float)sensor_value_to_double(&gyr[2]) * DEG_TO_RAD;

    madgwick_update_imu(
        &fusion,
        imu_state.gx,
        imu_state.gy,
        imu_state.gz,
        imu_state.ax,
        imu_state.ay,
        imu_state.az);
    madgwick_get_euler(&fusion, &imu_state.roll, &imu_state.pitch);
    k_mutex_unlock(&imu_mutex);
}

/*
 * Il driver Zephyr usa INT1 solo per DATA_READY (trigger_set asserisce
 * SENSOR_TRIG_DATA_READY) e possiede il pin. Il wake-up da movimento va quindi
 * configurato scrivendo direttamente i registri.
 *
 * enable=true : accel a 26 Hz, wake-up (slope filter) latched instradato su INT1,
 *               data-ready disabilitato su INT1.
 * enable=false: wake-up spento, data-ready su INT1 ripristinato, latch azzerato.
 */
static int imu_wakeup_config(bool enable)
{
    const struct {
        uint8_t reg;
        uint8_t val;
    } on[] = {
        {LSM6DSL_INT1_CTRL, 0x00},
        {LSM6DSL_CTRL1_XL, 0x20},                          /* 26 Hz, +/-2g */
        {LSM6DSL_WAKE_UP_THS, IMU_WAKEUP_THS & 0x3F},
        {LSM6DSL_WAKE_UP_DUR, 0x00},
        {LSM6DSL_TAP_CFG, BIT(7) | BIT(0)},                /* INTERRUPTS_ENABLE | LIR */
        {LSM6DSL_MD1_CFG, BIT(5)},                         /* INT1_WU */
    }, off[] = {
        {LSM6DSL_MD1_CFG, 0x00},
        {LSM6DSL_TAP_CFG, 0x00},
        {LSM6DSL_INT1_CTRL, BIT(1) | BIT(0)},              /* DRDY_G | DRDY_XL */
        {LSM6DSL_CTRL1_XL, 0x00},
    };
    const size_t n = enable ? ARRAY_SIZE(on) : ARRAY_SIZE(off);
    uint8_t src;
    int err;

    for (size_t i = 0; i < n; i++)
    {
        err = i2c_reg_write_byte_dt(&imu_i2c, enable ? on[i].reg : off[i].reg,
                                    enable ? on[i].val : off[i].val);
        if (err)
        {
            LOG_ERR("IMU reg write failed (%d)", err);
            return err;
        }
    }

    if (!enable)
    {
        /* La lettura di WAKE_UP_SRC rilascia il latch di INT1 */
        return i2c_reg_read_byte_dt(&imu_i2c, LSM6DSL_WAKE_UP_SRC, &src);
    }
    return 0;
}

int imu_arm_wakeup(void)
{
    uint8_t src;
    int err;

    imu_stop();

    err = imu_wakeup_config(true);
    if (err)
    {
        imu_wakeup_config(false);
        return err;
    }

    /* Lascia assestare lo slope filter, poi rilascia un eventuale latch spurio */
    k_sleep(K_MSEC(200));
    err = i2c_reg_read_byte_dt(&imu_i2c, LSM6DSL_WAKE_UP_SRC, &src);
    if (err || gpio_pin_get_dt(&imu_int1) > 0)
    {
        LOG_WRN("IMU in movimento, sleep annullato");
        imu_wakeup_config(false);
        return -EBUSY;
    }

    /* Il driver ha lasciato il pin con interrupt disabilitato (trigger NULL).
     * Il wake da System OFF richiede SENSE sul pin: lo imposto via HAL nRF,
     * il GPIO Zephyr non e' coinvolto. */
    nrf_gpio_cfg_sense_input(NRF_DT_GPIOS_TO_PSEL(IMU_NODE, irq_gpios),
                             NRF_GPIO_PIN_NOPULL,
                             NRF_GPIO_PIN_SENSE_HIGH);
    return 0;
}

int imu_init()
{
    if (!device_is_ready(imu_dev))
    {
        LOG_ERR("IMU device not ready");
        return -ENODEV;
    }

    sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    sensor_trigger_set(imu_dev, &trig_acc, NULL);
    sensor_trigger_set(imu_dev, &trig_gyro, NULL);

    /* Dopo un risveglio da System OFF l'IMU conserva la config di wake-up
     * (INT1 latched alto): la azzera e ripristina il data-ready su INT1. */
    imu_wakeup_config(false);

    madgwick_init(&fusion, FUSION_REFRESH_FREQ, FUSION_BETA);

    // k_timer_init(&imu_state_timer, imu_state_cb, NULL);
    k_timer_init(&imu_ble_timer, imu_ble_cb, NULL);

    LOG_INF("IMU subsystem ready");
    return 0;
}

void imu_start(void)
{
    // k_timer_start(&imu_state_timer, K_MSEC(0), K_MSEC(5));
    // LOG_INF("Started state timer");
    odr.val1 = 208;
    odr.val2 = 0;
    sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    sensor_trigger_set(imu_dev, &trig_acc, imu_data_ready_cb);
    sensor_trigger_set(imu_dev, &trig_gyro, imu_data_ready_cb);

    k_timer_start(&imu_ble_timer, K_MSEC(0), K_MSEC(20));
    LOG_INF("Started ble timer");
}

void imu_stop(void)
{
    // k_timer_stop(&imu_state_timer);
    // LOG_INF("Stopped state timer");
    odr.val1 = 0;
    odr.val2 = 0;
    sensor_attr_set(imu_dev, SENSOR_CHAN_ACCEL_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    sensor_attr_set(imu_dev, SENSOR_CHAN_GYRO_XYZ, SENSOR_ATTR_SAMPLING_FREQUENCY, &odr);
    sensor_trigger_set(imu_dev, &trig_acc, NULL);
    sensor_trigger_set(imu_dev, &trig_gyro, NULL);

    k_timer_stop(&imu_ble_timer);
    LOG_INF("Stopped ble timer");
}

void imu_ble_cb(struct k_timer *timer_id)
{
    k_mutex_lock(&imu_mutex, K_FOREVER);
    int offset = 0;

    int16_t ax = (int16_t)(imu_state.ax * 1000);
    int16_t ay = (int16_t)(imu_state.ay * 1000);
    int16_t az = (int16_t)(imu_state.az * 1000);
    int16_t gx = (int16_t)(imu_state.gx * 1000);
    int16_t gy = (int16_t)(imu_state.gy * 1000);
    int16_t gz = (int16_t)(imu_state.gz * 1000);

    // Int16: acc
    imu_pkt.buff[offset++] = ax & 0xFF;
    imu_pkt.buff[offset++] = (ax >> 8) & 0xFF;
    imu_pkt.buff[offset++] = ay & 0xFF;
    imu_pkt.buff[offset++] = (ay >> 8) & 0xFF;
    imu_pkt.buff[offset++] = az & 0xFF;
    imu_pkt.buff[offset++] = (az >> 8) & 0xFF;
    // Int16: gyro
    imu_pkt.buff[offset++] = gx & 0xFF;
    imu_pkt.buff[offset++] = (gx >> 8) & 0xFF;
    imu_pkt.buff[offset++] = gy & 0xFF;
    imu_pkt.buff[offset++] = (gy >> 8) & 0xFF;
    imu_pkt.buff[offset++] = gz & 0xFF;
    imu_pkt.buff[offset++] = (gz >> 8) & 0xFF;

    memcpy(imu_pkt.buff + offset, &imu_state.pitch, sizeof(float));
    offset += sizeof(float);
    memcpy(imu_pkt.buff + offset, &imu_state.roll, sizeof(float));

    imu_pkt.len = sizeof(imu_pkt.buff);

    if (k_msgq_put(&imu_queue, &imu_pkt, K_NO_WAIT) != 0)
    {
        LOG_WRN("IMU queue full, buffer dropped");
    }
    k_mutex_unlock(&imu_mutex);
}