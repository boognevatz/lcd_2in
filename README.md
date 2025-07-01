# lcd_2in


# .clangd

This is for hiding editor random errors (in Cursor/helix)

# compile_commands.json

Add 
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
into ports/rp2/CMakeLists.txt when bulding.

It will generate a compile_commands.json file inside build/ directory.
Copy it over to the project root.


