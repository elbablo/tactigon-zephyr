#include "led/led.h"

/* GPIO */

static const struct gpio_dt_spec led_r =
    GPIO_DT_SPEC_GET(DT_ALIAS(led_r), gpios);
static const struct gpio_dt_spec led_g =
    GPIO_DT_SPEC_GET(DT_ALIAS(led_g), gpios);
static const struct gpio_dt_spec led_b =
    GPIO_DT_SPEC_GET(DT_ALIAS(led_b), gpios);

static inline void led_on(const struct gpio_dt_spec *led)
{
    gpio_pin_set_dt(led, 1);
}

static inline void led_off(const struct gpio_dt_spec *led)
{
    gpio_pin_set_dt(led, 0);
}

static inline void leds_off(void)
{
    led_off(&led_r);
    led_off(&led_g);
    led_off(&led_b);
}

/* Stato */

static volatile led_mode_t current_mode = LED_MODE_OFF;

/* -------------------------------------------------- */
/* Effetti (non bloccanti) */

static void effect_heartbeat(void)
{
    static uint8_t step;
    static int64_t t0;
    int64_t now = k_uptime_get();

    if (now - t0 < 100) return;
    t0 = now;

    switch (step++) {
    case 0: led_on(&led_b);  break;
    case 1: led_off(&led_b); break;
    case 2: led_on(&led_b);  break;
    case 3: led_off(&led_b); break;
    default:
        step = 0;
        t0 = now + 600;      // pausa
        break;
    }
}

static void effect_connected(void)
{
    static uint8_t count;
    static int64_t t0;

    if (k_uptime_get() - t0 < 150) return;
    t0 = k_uptime_get();

    if (count < 4) {
        (count % 2 == 0) ? led_on(&led_g) : led_off(&led_g);
        count++;
    } else {
        led_off(&led_g);
        count = 0;
    }
}

static void effect_listening(void)
{
    static uint8_t idx;
    static int64_t t0;

    if (k_uptime_get() - t0 < 300) return;
    t0 = k_uptime_get();

    leds_off();

    switch (idx++ % 3) {
    case 0: led_on(&led_r); break;
    case 1: led_on(&led_g); break;
    case 2: led_on(&led_b); break;
    }
}

/* -------------------------------------------------- */
/* Timer */

static void led_timer_cb(struct k_timer *timer)
{
    switch (current_mode) {
    case LED_MODE_HEARTBEAT:
        effect_heartbeat();
        break;
    case LED_MODE_CONNECTED:
        effect_connected();
        break;
    case LED_MODE_LISTENING:
        effect_listening();
        break;
    case LED_MODE_OFF:
    default:
        leds_off();
        break;
    }
}

K_TIMER_DEFINE(led_timer, led_timer_cb, NULL);

/* -------------------------------------------------- */
/* API */

void led_init(void)
{
    gpio_pin_configure_dt(&led_r, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_g, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&led_b, GPIO_OUTPUT_INACTIVE);

    k_timer_start(&led_timer,
                  K_MSEC(50),
                  K_MSEC(50));
}

void led_set_mode(led_mode_t mode)
{
    current_mode = mode;
}