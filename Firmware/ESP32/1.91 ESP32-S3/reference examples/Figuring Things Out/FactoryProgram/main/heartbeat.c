#include "heartbeat.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "driver/gpio.h"

/* Timer handle */
static TimerHandle_t heartbeat_timer = NULL;

/* LED state */
static bool led_state = false;

/* Timer callback */
static void heartbeat_timer_callback(TimerHandle_t xTimer)
{
    led_state = !led_state;
    gpio_set_level(HEARTBEAT_GPIO, led_state);
}

void heartbeat_init(void)
{
    /* Configure GPIO */
    gpio_config_t io_conf = {
        .pin_bit_mask = 1ULL << HEARTBEAT_GPIO,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };

    gpio_config(&io_conf);

    /* Start LED OFF */
    gpio_set_level(HEARTBEAT_GPIO, 0);
    led_state = false;

    /* Create timer (60ms heartbeat) */
    heartbeat_timer = xTimerCreate(
        "heartbeat_timer",
        pdMS_TO_TICKS(65),
        pdTRUE,     // auto reload
        NULL,
        heartbeat_timer_callback
    );
}

void heartbeat_start(void)
{
    if (heartbeat_timer != NULL) {
        xTimerStart(heartbeat_timer, 0);
    }
}

void heartbeat_stop(void)
{
    if (heartbeat_timer != NULL) {
        xTimerStop(heartbeat_timer, 0);
        gpio_set_level(HEARTBEAT_GPIO, 0);
        led_state = false;
    }
}
