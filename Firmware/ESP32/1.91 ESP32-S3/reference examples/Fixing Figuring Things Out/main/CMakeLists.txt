idf_component_register(
    SRCS 
        "example_qspi_with_ram.c"
        "ui/ui.c"
        "ui/ui_events.c"
        "ui/ui_helpers.c"
        "ui/ui_Screen1.c"
        "ui/ui_img_zoetrope_splash_screen2_png.c"
        "heartbeat.c"
        "uart_protocol.c"
        "platter_ui.c"
        "ui_highlight.c"
        "auto_scroll.c"
    INCLUDE_DIRS
        "ui" 
        "."
)

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)