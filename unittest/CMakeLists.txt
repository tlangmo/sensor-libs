cmake_minimum_required(VERSION 3.2)
project (motesque_unittest)

include_directories(${CURRENT_SOURCE_DIR} )

add_executable(run_tests main.cpp 
               $<TARGET_OBJECTS:motesque_test_lib_http>
                $<TARGET_OBJECTS:motesque_test_lib_lw_event_trace>
                $<TARGET_OBJECTS:motesque_test_lib_message>
                $<TARGET_OBJECTS:motesque_test_lib_util>
                $<TARGET_OBJECTS:motesque_test_lib_rbs>
                $<TARGET_OBJECTS:motesque_test_lib_slotfs>
                $<TARGET_OBJECTS:motesque_test_lib_discovery>
               )

