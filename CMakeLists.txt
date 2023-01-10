cmake_minimum_required(VERSION 3.2)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -std=c++1y -pthread -g -Wall") # -std=c++1y probably isn't mandatory

add_subdirectory(lib_http/tests/)
add_subdirectory(lib_util/tests/)
add_subdirectory(lib_slotfs/tests/)
add_subdirectory(lib_rbs/tests/)
add_subdirectory(lib_lw_event_trace/tests/)
add_subdirectory(lib_message/tests/)
add_subdirectory(lib_discovery/tests/)
add_subdirectory(unittest)

