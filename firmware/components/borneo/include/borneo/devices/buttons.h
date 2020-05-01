#pragma once

#ifdef __cplusplus
extern "C" {
#endif
/* Declarations of this file */

ESP_EVENT_DECLARE_BASE(BORNEO_BUTTON_EVENTS);

enum {
    BORNEO_EVENT_BUTTON_PRESSED = 1,
    BORNEO_EVENT_BUTTON_LONG_PRESSED,
};

typedef enum {
    BUTTON_RELEASE = 0,
    BUTTON_SHORT_PRESSED = 1,
    BUTTON_LONG_PRESSED = 2,
} ButtonState;

typedef struct {
    int id;
    int io_pin;
    uint32_t pressed_on;
} SimpleButtonStatus;

typedef struct {
    SimpleButtonStatus* buttons;
    size_t size;
} SimpleButtonGroupStatus;

typedef struct {
    uint32_t id;
    int io_pin;
} SimpleButton;

int SimpleButtonGroup_init(const SimpleButton* buttons, size_t n);
int SimpleButtonGroup_start();

#ifdef __cplusplus
}
#endif
