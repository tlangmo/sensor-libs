
#include "wiced.h"

namespace motesque {

// return current time in milliseconds
uint32_t nowMs() {
    return host_rtos_get_time();
}

}; // end ns
