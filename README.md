# Motesque Sensor Libraries

This Repository is a collection of platform independent libraries used in Motesque's IMU sensor. 

## Libraries

### lib_rbs
Messaging and server for Reference Broadcast Syncing, a method to synchronize multiple sensor in a wireless network

### lib_message
Templates for flexible IMU data messages. Individual data structs can be assembled in various order, and transmitted with typing information

### lib_lw_event_trace
A lightweight implementation of Chrome's event tracing mechanism
Files created can be viewed in [Chrome's about:tracing](https://www.chromium.org/developers/how-tos/trace-event-profiling-tool/)

### lib_slot_fs
A simple filesystem for block storages on embedded devices. It is focussed on robustness against power outage, code size and simplicity.

### lib_http
A specialized embedded http server which backs the sensors RestAPI. It allows for efficient data responses, supports several MIME types, and http range requests. The server code was written for Cypress's WicedSDK, but the general code can be adapted to Berkely sockets easily.

### lib_util
Various utility functions and classes. These include thread-safe dispatch queue (CommandQueue) and a thread safe sequential buffering class. 


## Requiremens
* C++ 11 Compiler Toolchain
* Posix-like OS, such as Linux or MacOS


## Build and run unit tests
```
./create-project.sh
cd build_x64
make
./unitests/run_tests
```