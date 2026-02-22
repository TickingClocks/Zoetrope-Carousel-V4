#ifndef GPIO_BSP_H
#define GPIO_BSP_H

#define example_test_out1 16
#define example_test_out2 17
#define example_test_in1 5
#define example_test_in2 6
#define example_key      0

void esp32_gpio_init(void);
void GPIO_SET(uint8_t pin,uint8_t mode);
uint8_t GPIO_GET(uint8_t pin);
#endif