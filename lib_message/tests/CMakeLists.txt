
set(SOURCES 
    ../message_frame.cpp
    message_frame.t.cpp

)
add_library(motesque_test_lib_message OBJECT ${SOURCES})

# Make sure the compiler can find the include files 
target_include_directories (motesque_test_lib_message PUBLIC
                            ${CMAKE_SOURCE_DIR}/lib_tests/
                            ${CMAKE_SOURCE_DIR}/lib_message/
                            )
