
set(SOURCES 
    ../http_request.cpp   
    ../http_response.cpp    
    ../http_payload.cpp   
    ../http_utils.cpp 
    ../http_parser.c    
    http_request.t.cpp
    http_payload_file.t.cpp
    http_response.t.cpp

)
# definitions to compile on x86 instead of wiced
add_definitions(-DMOTESQUE_PLATFORM_X86)

add_library(motesque_test_lib_http OBJECT ${SOURCES})

# Make sure the compiler can find the include files 
target_include_directories (motesque_test_lib_http PUBLIC 
                            ${CMAKE_SOURCE_DIR}/lib_tests/
                            ${CMAKE_SOURCE_DIR}/lib_http/
                            ${CMAKE_SOURCE_DIR}/lib_util/
                            )