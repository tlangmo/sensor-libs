#pragma once
#include "wiced_rtos.h"

struct EventFlagStatus {
    EventFlagStatus() :
        event_flags() {

        wiced_result_t rc = wiced_rtos_init_event_flags(&event_flags);
        if (rc != WICED_SUCCESS) {
             WPRINT_APP_INFO(("wiced_rtos_init_event_flags failed, rc=%d\n",rc));
        }
    }
    ~EventFlagStatus() {
        wiced_rtos_deinit_event_flags(&event_flags);
    }

    void set() {
        wiced_result_t rc = wiced_rtos_set_event_flags(&event_flags, 1);
        if (rc != WICED_SUCCESS) {
             WPRINT_APP_INFO(("wiced_rtos_set_event_flags failed, rc=%d\n",rc));
        }
    }

    void clear() {
        // We need to use the ThreadX interface directly here because `wiced_rtos_set_event_flags` cannot be 
        // used to "unset" flags! 
        tx_event_flags_set(&event_flags,0, TX_AND);
    }

    int wait_for(uint32_t timeout_ms) {
        uint32_t flags_set = 0;
        wiced_result_t rc = wiced_rtos_wait_for_event_flags(&event_flags, 1 ,&flags_set, WICED_TRUE, WAIT_FOR_ANY_EVENT, 2000);
        return (rc == WICED_SUCCESS) ? 0: -1;
    }
    wiced_event_flags_t event_flags;
};
