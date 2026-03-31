# Create an INTERFACE library for our C module.
add_library(usermod_camera INTERFACE)

# Add our source files to the lib
target_sources(usermod_camera INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/cam.c
    ${CMAKE_CURRENT_LIST_DIR}/modcamera.c
    ${CMAKE_CURRENT_LIST_DIR}/ov5640.c
    ${CMAKE_CURRENT_LIST_DIR}/picampinos.pio
    ${MICROPY_DIR}/lib/wiznet5k/Ethernet/socket.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_camera INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
    ${MICROPY_DIR}
    ${MICROPY_DIR}/extmod
)


# Link to the Pico SDK
target_link_libraries(usermod_camera INTERFACE
    pico_stdlib
    hardware_spi
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_camera)
