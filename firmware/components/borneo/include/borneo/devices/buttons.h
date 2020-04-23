#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

ESP_EVENT_DECLARE_BASE(BORNEO_BUTTON_EVENTS);


enum {
    BORNEO_EVENT_BUTTON_PUSHED = 1,
};


int SimplePushButton_init(int io_pin);

#ifdef __cplusplus
}
#endif
