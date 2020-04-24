#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <string.h>
#include <sys/param.h>
#include <tcpip_adapter.h>

#include <esp32/clk.h>
#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>
#include <lwip/udp.h>

#include "borneo/broadcast.h"
#include "borneo/device-config.h"
#include "borneo/rtc.h"
#include "borneo/serial.h"

static const char* TAG = "UDP";

#define HOST_IP_ADDR "255.255.255.255"

const char* MSG_FORMAT = "{\"MagicWord\":9966,\"Message\":{\"Ts\":%d,\"TcpPort\":%d,\"Serial\":\"%"
                         "s\",\"DevName\":\"%s\"}}";

#define MAX_PACKET_SIZE 1024

uint8_t s_packet_buf[MAX_PACKET_SIZE];

static void udp_client_task(void* params)
{
    struct pbuf* p;

    struct udp_pcb* udp = udp_new();

    p = pbuf_alloc(PBUF_TRANSPORT, MAX_PACKET_SIZE, PBUF_RAM);

    const TickType_t freq = 5000 / portTICK_PERIOD_MS;
    TickType_t last_wake_time = xTaskGetTickCount();
    for (;;) {
        memset(s_packet_buf, 0, MAX_PACKET_SIZE);
        snprintf((char*)s_packet_buf, MAX_PACKET_SIZE - 1, MSG_FORMAT, Rtc_timestamp(), BORNEO_DEVICE_TCP_PORT,
            Serial_get(), BORNEO_DEVICE_NAME);

        p->payload = s_packet_buf;
        p->tot_len = p->len = strlen((const char*)s_packet_buf);

        // Allocate packet buffer
        udp_sendto(udp, p, IP_ADDR_BROADCAST, BORNEO_DEVICE_UDP_PORT);

        vTaskDelayUntil(&last_wake_time, freq);
    }

    vTaskDelete(NULL);


    
    pbuf_free(p); // De-allocate packet buffer
    udp_remove(udp);
    vTaskDelete(NULL);
}

int Broadcast_init()
{
    ESP_LOGI(TAG, "Initailzing UDP broadcasting....");
    return 0;
}

int Broadcast_start()
{
    /* This helper function configures Wi-Fi or Ethernet, as selected in
     * menuconfig. Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_LOGI(TAG, "Starting UDP broadcasting....");

    xTaskCreate(udp_client_task, "udp_client", 4096, NULL, tskIDLE_PRIORITY, NULL);
    ESP_LOGI(TAG, "UDP broadcasting started.");
    return 0;
}