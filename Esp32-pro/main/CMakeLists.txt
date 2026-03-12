idf_component_register(
    SRCS
        "main.c"
        "bt_core.c"
        "bt_comms.c"
        "bt_reports.c"
        "bt_spi.c"
        "bt_input.c"
        "bt_settings.c"
    INCLUDE_DIRS "."
    REQUIRES
        driver
        nvs_flash
        bt
)
