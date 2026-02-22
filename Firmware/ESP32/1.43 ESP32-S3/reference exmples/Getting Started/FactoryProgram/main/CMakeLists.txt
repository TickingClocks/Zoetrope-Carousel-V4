idf_component_register(
    SRCS 
        "example_qspi_with_ram.c"
        "ui/ui_comp_hook.c"
        "ui/ui_helpers.c"
        "ui/ui_screen1.c"
        "ui/ui.c"
        "heartbeat.c"
    INCLUDE_DIRS 
        "."
        "ui"
        )

set_source_files_properties(
    ${LV_DEMOS_SOURCES}
    PROPERTIES COMPILE_OPTIONS
    -DLV_LVGL_H_INCLUDE_SIMPLE)
