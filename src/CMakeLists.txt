cmake_minimum_required (VERSION 3.16)

# A interface to reuse common properties.
# You can add more external include paths of other projects that are needed for your project.
add_library ("${PROJECT_NAME}_interface" INTERFACE)
target_link_libraries ("${PROJECT_NAME}_interface" INTERFACE seqan3::seqan3)
target_include_directories ("${PROJECT_NAME}_interface" INTERFACE ../include)
target_include_directories ("${PROJECT_NAME}_interface" INTERFACE ../lib/valik/include)
target_compile_options ("${PROJECT_NAME}_interface" INTERFACE "-pedantic" "-Wall" "-Wextra")

add_executable ("${PROJECT_NAME}" main.cpp)
target_link_libraries ("${PROJECT_NAME}" PRIVATE "${PROJECT_NAME}_interface")
