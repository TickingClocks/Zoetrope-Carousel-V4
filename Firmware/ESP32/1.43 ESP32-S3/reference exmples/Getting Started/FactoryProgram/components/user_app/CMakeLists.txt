idf_component_register(
  SRCS "user_app.c"
  PRIV_REQUIRES touch_bsp pcf85063 qmi8658c adc_bsp sd_card_bsp ble_scan_bsp esp_wifi_bsp gpio_bsp button_bsp
  REQUIRES ui_bsp
  INCLUDE_DIRS "./")
