idf_component_register(SRCS "pn532_rfid.c" "main.c" "door_control.c"
                    INCLUDE_DIRS ".")

target_compile_options(${COMPONENT_LIB} PRIVATE -Wall)