#include <stdio.h>
#include "gpio_bsp.h"
#include "driver/gpio.h"
#include "esp_err.h"

#define bit_mask (uint64_t)0x01

void esp32_gpio_init(void)
{
  gpio_config_t gpio_conf = {};
  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_OUTPUT;
  gpio_conf.pin_bit_mask = (bit_mask<<example_test_out1) | (bit_mask<<example_test_out2);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf)); //ESP32 onboard GPIO

  gpio_conf.intr_type = GPIO_INTR_DISABLE;
  gpio_conf.mode = GPIO_MODE_INPUT;
  gpio_conf.pin_bit_mask = (bit_mask<<example_test_in1) | (bit_mask<<example_test_in2) | (bit_mask<<example_key);
  gpio_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_conf.pull_up_en = GPIO_PULLUP_ENABLE;

  ESP_ERROR_CHECK_WITHOUT_ABORT(gpio_config(&gpio_conf));
}
void GPIO_SET(uint8_t pin,uint8_t mode)
{
  gpio_set_level(pin,mode);
}
uint8_t GPIO_GET(uint8_t pin)
{
  return gpio_get_level(pin);
}
