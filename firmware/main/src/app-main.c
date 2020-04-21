

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <cJSON.h>

#include <esp32/clk.h>
#include <esp_event.h>
#include <esp_log.h>
#include <esp_smartconfig.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/FreeRTOSConfig.h>
#include <freertos/event_groups.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <tcpip_adapter.h>

#include "borneo/broadcast.h"
#include "borneo/cron.h"
#include "borneo/devices/buttons.h"
#include "borneo/devices/leds.h"
#include "borneo-doser/devices/pump.h"
#include "borneo/devices/wifi.h"
#include "borneo/rpc.h"
#include "borneo/rtc.h"
#include "borneo-doser/scheduler.h"
#include "borneo/serial.h"
#include "borneo/sntp.h"

#include "borneo/rpc/sys.h"

#include "borneo-doser/rpc/doser.h"

static int App_init_devices();
static void App_idle_task();

const RpcMethodEntry RPC_METHOD_TABLE[] = {
    { .name = "sys.hello", .callback = &RpcMethod_sys_hello },
    { .name = "doser.pump_until", .callback = &RpcMethod_doser_pump_until },
    { .name = "doser.pump", .callback = &RpcMethod_doser_pump },
    { .name = "doser.speed_set", .callback = &RpcMethod_doser_speed_set },
    { .name = "doser.schedule_get", .callback = &RpcMethod_doser_schedule_get },
    { .name = "doser.schedule_set", .callback = &RpcMethod_doser_schedule_set },
};

static void disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {}

static void connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    ESP_LOGI("APP", "Connected to station.");
    ESP_ERROR_CHECK(Broadcast_init());
    ESP_ERROR_CHECK(Broadcast_start());

    ESP_ERROR_CHECK(Rpc_init(RPC_METHOD_TABLE, sizeof(RPC_METHOD_TABLE) / sizeof(RpcMethodEntry)));
    ESP_ERROR_CHECK(Rpc_start());

    ESP_ERROR_CHECK(Sntp_init());
    // 上电有网络就执行一次 SNTP 时间同步
    ESP_ERROR_CHECK(Sntp_try_sync_time());

    OnboardLed_start_slow_blink();
}

static int App_init_devices()
{
    ESP_ERROR_CHECK(OnboardLed_init());

    // 设备初始化自检开始，让板载 LED 常亮
    OnboardLed_on();

    ESP_ERROR_CHECK(Serial_init());

    ESP_ERROR_CHECK(Pump_init());

    // 初始化并启动 RTC
    ESP_ERROR_CHECK(Rtc_init());
    ESP_ERROR_CHECK(Rtc_start());

    // 初始化并启动 Scheduler
    ESP_ERROR_CHECK(Scheduler_init());
    ESP_ERROR_CHECK(Scheduler_start());

    return 0;
}

/**
 * 辅助任务进程，处理指示灯等不重要且不需要实时的任务
 */
static void App_idle_task()
{
    const TickType_t freq = 25 / portTICK_PERIOD_MS;
    TickType_t last_wake_time = xTaskGetTickCount();
    for (;;) {
        // 驱动板载 LED
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        OnboardLed_drive(now);
        vTaskDelayUntil(&last_wake_time, freq);
    }

    vTaskDelete(NULL);
}

void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(App_init_devices());

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // 启动辅助进程
    xTaskCreate(App_idle_task, "idle_task", 4096, NULL, tskIDLE_PRIORITY, NULL);

    OnboardLed_start_fast_blink();

    ESP_ERROR_CHECK(Wifi_init());

    int result = Wifi_try_connect();
    if (result != ESP_OK) {
        ESP_ERROR_CHECK(Wifi_smartconfig_and_wait());
    }

    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &connect_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &disconnect_handler, NULL));
}
