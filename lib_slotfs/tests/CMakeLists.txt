
set(SOURCES 
    ../slotfs.cpp   
    ../../lib_util/util_crc32.cpp   
    slotfs.t.cpp
)

# definitions to compile on x86 instead of wiced
add_definitions(-DMOTESQUE_PLATFORM_X86)

add_library(motesque_test_lib_slotfs OBJECT ${SOURCES})

# Make sure the compiler can find the include files 
target_include_directories (motesque_test_lib_slotfs PUBLIC 
                            ${CMAKE_SOURCE_DIR}/lib_tests/
                            ${CMAKE_SOURCE_DIR}/lib_util/
                            ${CMAKE_SOURCE_DIR}/lib_lw_event_trace/
                            ${CMAKE_SOURCE_DIR}/lib_slotfs/
                            )