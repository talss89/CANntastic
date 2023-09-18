idf_component_register(SRCS "main.c" 
    "control_event.c" 
    "control_scheme.c" 
    "canbus.c" 
    "gatt_svr.c"
    "gatt_vars.c"
    "ble_hid.c" 
    "hid_func.c"
    "dns_server.c"
    "web_config.c"
    "scheme/basic.c"
    "scheme/webconfig.c"
    INCLUDE_DIRS "."
    )

# The following custom targets build C headers from JSON configuration.

add_custom_command(OUTPUT controls.h 
    WORKING_DIRECTORY ${COMPONENT_DIR}/../webconfig
    COMMAND npm run build && cp ./dist-embedded/external/src/controls.h ../main/controls.h
    DEPENDS ${COMPONENT_DIR}/../webconfig/src/controls.xd.json
    VERBATIM)

add_custom_target(bud_build DEPENDS controls.h)
add_dependencies(${COMPONENT_LIB} bud_build)

set_property(DIRECTORY "${COMPONENT_DIR}" APPEND PROPERTY
     ADDITIONAL_CLEAN_FILES controls.h)


# Build the web config application, and embed the index.html.gz file into the firmware

add_custom_target(webconfig COMMAND npm run build WORKING_DIRECTORY ${COMPONENT_DIR}/../webconfig)
target_add_binary_data(${COMPONENT_LIB} ${COMPONENT_DIR}/../webconfig/dist-embedded/external/build/index.html.gz BINARY DEPENDS bud_build ${COMPONENT_DIR}/../webconfig/dist-embedded/index.html)
