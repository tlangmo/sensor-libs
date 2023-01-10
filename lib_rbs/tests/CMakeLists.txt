
set(SOURCES 
   ../rbs_message.cpp
   ../../lib_util/device_info.cpp
   ../rbs_sync_table.cpp
    rbs_message.t.cpp
    rbs_sync_table.t.cpp

)
# definitions to compile on x86 instead of wiced
add_definitions(-DMOTESQUE_PLATFORM_X86)

add_library(motesque_test_lib_rbs OBJECT ${SOURCES})

# Make sure the compiler can find the include files 
target_include_directories (motesque_test_lib_rbs PUBLIC 
                            ${CMAKE_SOURCE_DIR}/lib_tests/
                            ${CMAKE_SOURCE_DIR}/lib_rbs/
                            ${CMAKE_SOURCE_DIR}/lib_util/
                            )