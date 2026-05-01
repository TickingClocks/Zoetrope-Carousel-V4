idf_component_register(
  SRCS "user_app.c"
  PRIV_REQUIRES i2c_bsp pcf85063
  INCLUDE_DIRS "./")
