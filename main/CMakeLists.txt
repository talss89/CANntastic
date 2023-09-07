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


# Build the web config application, and embed the index.html.gz file into the firmware

add_custom_target(webconfig COMMAND npm run build WORKING_DIRECTORY ${COMPONENT_DIR}/../webconfig)
target_add_binary_data(${COMPONENT_LIB} ${COMPONENT_DIR}/../webconfig/dist/index.html.gz BINARY DEPENDS webconfig ${COMPONENT_DIR}/../webconfig/src/app.html ${COMPONENT_DIR}/../webconfig/src/index.css ${COMPONENT_DIR}/../webconfig/src/index.ts)

# The following custom targets build C headers from JSON configuration.

add_custom_command(OUTPUT controls.h
    COMMAND node ${COMPONENT_DIR}/../util/cross-defs/index.js -i ${COMPONENT_DIR}/../x_defs/controls.json -c ${COMPONENT_DIR}/controls.h -j ${COMPONENT_DIR}/../webconfig/src/controls.json
    DEPENDS ${COMPONENT_DIR}/../x_defs/controls.json
    VERBATIM)

add_custom_target(controls_xdef DEPENDS controls.h)
add_dependencies(${COMPONENT_LIB} controls_xdef)

set_property(DIRECTORY "${COMPONENT_DIR}" APPEND PROPERTY
     ADDITIONAL_CLEAN_FILES controls.h ${COMPONENT_DIR}/../webconfig/dist/index.html.gz)