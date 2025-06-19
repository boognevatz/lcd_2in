# Create an INTERFACE library for our C module.
add_library(usermod_lcd INTERFACE)

# Add our source files to the lib
target_sources(usermod_lcd INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/DEV_Config.c
    ${CMAKE_CURRENT_LIST_DIR}/LCD_2in.c
)

# Add the current directory as an include directory.
target_include_directories(usermod_lcd INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)


# Link to the Pico SDK
target_link_libraries(usermod_lcd INTERFACE
    pico_stdlib
    hardware_spi
)

# Link our INTERFACE library to the usermod target.
target_link_libraries(usermod INTERFACE usermod_lcd)
