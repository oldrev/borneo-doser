#include "esp_system.h"
#include <esp_err.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "borneo/serial.h"
#include "borneo/utils/md5.h"

const uint8_t s_magic[4] = { 0x64, 0x89, 0x99, 0x20 };
static char s_serial[33];

int Serial_init()
{
    uint8_t serial_bytes[10];
    memset(serial_bytes, 0, sizeof(serial_bytes));
    int ret = esp_efuse_mac_get_default(serial_bytes);
    if (ret != ESP_OK) {
        return ret;
    }
    for (size_t i = 0; i < 4; i++) {
        serial_bytes[6 + i] = s_magic[i] + 19;
    }
    memset(s_serial, 0, sizeof(s_serial));

    ret = MD5_compute_buffer_to_hex(serial_bytes, sizeof(serial_bytes), s_serial);
    return ret;
}

char* Serial_get() { return s_serial; }
