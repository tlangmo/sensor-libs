
set(SOURCES 
    discovery.t.cpp
    ../../lib_util/ip_address.cpp   
)


add_library(motesque_test_lib_discovery OBJECT ${SOURCES})

# Make sure the compiler can find the include files 
target_include_directories (motesque_test_lib_discovery PUBLIC 
                            ${CMAKE_SOURCE_DIR}/lib_tests/
                            ${CMAKE_SOURCE_DIR}/lib_util/
                            ${CMAKE_SOURCE_DIR}/lib_discovery/
                            )