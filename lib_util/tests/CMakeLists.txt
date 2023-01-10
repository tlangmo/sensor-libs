
set(SOURCES 
    command_queue.t.cpp
    sequential_buffer.t.cpp
    generic_sensor.t.cpp
    ../json_frozen.c
    md5.t.cpp
     motesque_version.t.cpp
    ../md5.c

)
# definitions to compile on x86 instead of wiced
add_definitions(-DMOTESQUE_PLATFORM_X86)

add_library(motesque_test_lib_util OBJECT ${SOURCES})

# Make sure the compiler can find the include files 
target_include_directories (motesque_test_lib_util PUBLIC 
                            ${CMAKE_SOURCE_DIR}/lib_tests/
                            ${CMAKE_SOURCE_DIR}/lib_util/
                            )