#ifndef TOUCH_BSP_H
#define TOUCH_BSP_H


void Touch_Init(void);
uint8_t getTouch(uint16_t *x,uint16_t *y);
uint8_t I2C_writr_buff(uint8_t addr,uint8_t reg,uint8_t *buf,uint8_t len);
uint8_t I2C_read_buff(uint8_t addr,uint8_t reg,uint8_t *buf,uint8_t len);
uint8_t I2C_read_buff(uint8_t addr,uint8_t reg,uint8_t *buf,uint8_t len);
#endif
