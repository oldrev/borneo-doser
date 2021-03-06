

#include <math.h>
#include <stdlib.h>
#include <string.h>

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
#include <driver/gpio.h>

#include "borneo/common.h"
#include "borneo/broadcast.h"
#include "borneo/cron.h"
#include "borneo/devices/leds.h"
#include "borneo/devices/buttons.h"
#include "borneo-doser/devices/pump.h"
#include "borneo/devices/wifi.h"
#include "borneo/rpc.h"
#include "borneo/rtc.h"
#include "borneo-doser/scheduler.h"
#include "borneo/devices/buttons.h"
#include "borneo/serial.h"
#include "borneo/sntp.h"

#include "borneo/rpc/sys.h"

#include "borneo-doser/rpc/doser.h"

static int App_init_core_devices();
static void App_idle_task();
static void init_network_task();

static const char* TAG = "APP_MAIN";

const RpcMethodEntry RPC_METHOD_TABLE[] = {
    { .name = "sys.hello", .callback = &RpcMethod_sys_hello },
    { .name = "doser.pump_until", .callback = &RpcMethod_doser_pump_until },
    { .name = "doser.pump", .callback = &RpcMethod_doser_pump },
    { .name = "doser.speed_set", .callback = &RpcMethod_doser_speed_set },
    { .name = "doser.schedule_get", .callback = &RpcMethod_doser_schedule_get },
    { .name = "doser.schedule_set", .callback = &RpcMethod_doser_schedule_set },
    { .name = "doser.status", .callback = &RpcMethod_doser_status },
};

const SimpleButton SIMPLE_BUTTONS[] = { { .id = 0, .io_pin = 27 } };

static void wifi_disconnect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // WiFi 断开了，让指示灯快闪
    OnboardLed_start_fast_blink();
}

static void wifi_connect_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // 已正常连接 WiFi，让指示灯慢速闪亮
    OnboardLed_start_slow_blink();

    ESP_LOGI(TAG, "Connected to station.");

    // 初始化依赖网络的部件
    xTaskCreate(init_network_task, "init_network_task", 4096, NULL, tskIDLE_PRIORITY + 1, NULL);
}

static void on_button_pushed(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    int button_id = *((int*)event_data);
    ESP_LOGI(TAG, "Button %d pressed", button_id);
    OnboardLed_start_fast_blink();
}

static void on_button_long_pushed(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    // 上电 100 秒内长按按钮将恢复转入待配网状态
    int button_id = *((int*)event_data);
    ESP_LOGI(TAG, "Button %d long pressed", button_id);
    uint64_t now = esp_timer_get_time() / 1000000ULL;
    if (button_id == 0 && now < 100) {
        OnboardLed_start_fast_blink();
        // 重新恢复出厂设置
        Wifi_restore_and_reboot();
    }
}

static int App_init_core_devices()
{

    ESP_ERROR_CHECK(OnboardLed_init());

    // 核心设备初始化自检开始，让板载 LED 关闭
    OnboardLed_off();

    // 初始化模式按钮
    ESP_ERROR_CHECK(SimpleButtonGroup_init(SIMPLE_BUTTONS, 1)); // IO27
    ESP_ERROR_CHECK(
        esp_event_handler_register(BORNEO_BUTTON_EVENTS, BORNEO_EVENT_BUTTON_PRESSED, &on_button_pushed, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(
        BORNEO_BUTTON_EVENTS, BORNEO_EVENT_BUTTON_LONG_PRESSED, &on_button_long_pushed, NULL));
    ESP_ERROR_CHECK(SimpleButtonGroup_start());

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
    const TickType_t freq = 100 / portTICK_PERIOD_MS;
    TickType_t last_wake_time = xTaskGetTickCount();
    for (;;) {
        // 驱动板载 LED
        uint32_t now = xTaskGetTickCount() * portTICK_PERIOD_MS;
        OnboardLed_drive(now);

        // 驱动 SNTP 定时器
        Sntp_drive_timer();

        vTaskDelayUntil(&last_wake_time, freq);
    }

    vTaskDelete(NULL);
}

void app_main()
{
    // Initialize NVS.
    esp_err_t err = nvs_flash_init();
    // 如果 NVS 格式更新了就只有擦除了
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // 启动主事件循环
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    gpio_install_isr_service(0);

    ESP_ERROR_CHECK(App_init_core_devices());

    // 核心设备初始化完毕，可以启动辅助进程
    xTaskCreate(App_idle_task, "idle_task", 4096, NULL, tskIDLE_PRIORITY, NULL);

    // 让指示灯快闪，表示开始网络初始化
    OnboardLed_start_fast_blink();

    // 挂接 WiFi 事件
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_connect_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifi_disconnect_handler, NULL));

    // 初始化 WiFi
    ESP_ERROR_CHECK(Wifi_init());
    ESP_ERROR_CHECK(Wifi_start());

}


static void init_network_task(void* params) {
    // 初始化 UDP 广播
    ESP_ERROR_CHECK(Broadcast_init());
    ESP_ERROR_CHECK(Broadcast_start());

    // 初始化 RPC 子系统
    ESP_ERROR_CHECK(Rpc_init(RPC_METHOD_TABLE, sizeof(RPC_METHOD_TABLE) / sizeof(RpcMethodEntry)));
    ESP_ERROR_CHECK(Rpc_start());

    // 初始化 SNTP 部件
    ESP_ERROR_CHECK(Sntp_init());
    // 上电有网络就执行一次 SNTP 时间同步
    ESP_ERROR_CHECK(Sntp_try_sync_time());

    vTaskDelete(NULL);
}