
set(SOURCES 
    ../lw_event_trace.cpp
    ../lw_event_trace_platform_x86.cpp
    event_trace.t.cpp

)
# definitions to compile on x86 instead of wiced
add_definitions(-DMOTESQUE_PLATFORM_X86 -DMOTESQUE_ENABLE_EVENT_TRACE)

add_library(motesque_test_lib_lw_event_trace OBJECT ${SOURCES})

# Make sure the compiler can find the include files 
target_include_directories (motesque_test_lib_lw_event_trace PUBLIC
                            ${CMAKE_SOURCE_DIR}/lib_tests/
                            ${CMAKE_SOURCE_DIR}/lib_lw_event_trace/
                            )
