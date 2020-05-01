#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <driver/gpio.h>
#include <driver/periph_ctrl.h>
#include <esp32/clk.h>
#include <esp32/rom/ets_sys.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <esp_types.h>
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <nvs.h>
#include <nvs_flash.h>

#include "borneo/common.h"
#include "borneo-doser/devices/pump.h"

#define PUMP_TIMER_GROUP TIMER_GROUP_1
#define PUMP_TIMER_INDEX 0

typedef struct {
    volatile PumpState state; // 状态
    volatile int duration; // 任务持续时间，单位毫秒
    esp_timer_handle_t timer; // 任务定时器
    esp_timer_create_args_t timer_args; // 任务定时器参数
} PumpChannel;

typedef struct {
    PumpChannel channels[PUMP_MAX_CHANNELS];
    PumpDeviceConfig config;
} PumpStatus;

static int start_timer();
static int prepare_for_duration(int ch, int duration);
static int prepare_for_volume(int ch, double vol);
static void timer_callback(void* params);
static int save_config();
static int load_config();

const PumpPort PUMP_PORT_TABLE[] = {
    { .name = "P1", .io_pin = 32 },
    { .name = "P2", .io_pin = 33 },
    { .name = "P3", .io_pin = 25 },
    { .name = "P4", .io_pin = 26 },
};

static PumpStatus s_pump_status;

static const char* TAG = "PUMP";

static const char* NVS_NAMESPACE = "pump";
static const char* NVS_PUMP_CONFIG_KEY = "config";

static const PumpDeviceConfig PUMP_DEFAULT_CONFIG = { .speeds = { 12.0, 12.0, 12.0, 12.0 } };

int Pump_init()
{
    memset(&s_pump_status, 0, sizeof(s_pump_status));

    uint64_t pins_mask = 0;
    for (size_t i = 0; i < sizeof(PUMP_PORT_TABLE) / sizeof(PumpPort); i++) {
        pins_mask |= (1ULL << PUMP_PORT_TABLE[i].io_pin);
    }

    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_PIN_INTR_DISABLE; // 禁止中断
    io_conf.mode = GPIO_MODE_OUTPUT; // 输出模式
    io_conf.pin_bit_mask = pins_mask; // 选定端口
    io_conf.pull_down_en = 1; // 打开下拉
    io_conf.pull_up_en = 0; // 禁止上拉
    gpio_config(&io_conf);

    // 确保全部都是关闭的
    for (size_t i = 0; i < sizeof(PUMP_PORT_TABLE) / sizeof(PumpPort); i++) {
        Pump_off(i);

        // 初始化定时器

        // 设置参数
        PumpChannel* pc = &s_pump_status.channels[i];
        pc->timer_args.callback = &timer_callback;
        pc->timer_args.arg = (void*)i;
        pc->timer_args.name = PUMP_PORT_TABLE[i].name;

        ESP_ERROR_CHECK(esp_timer_create(&pc->timer_args, &pc->timer));
    }

    // 加载
    if (load_config() == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "Saving default config...");
        memcpy(&s_pump_status.config, &PUMP_DEFAULT_CONFIG, sizeof(s_pump_status.config));
        save_config();
    }

    return 0;
}

int Pump_start(int ch, double vol)
{
    int error = prepare_for_volume(ch, vol);
    if (error != 0) {
        return error;
    }
    return start_timer();
}

int Pump_start_until(int ch, int ms)
{
    int error = prepare_for_duration(ch, ms);
    if (error != 0) {
        return error;
    }
    return start_timer();
}

int Pump_start_all(const double* vols)
{
    int error = 0;
    for (size_t i = 0; i < sizeof(PUMP_PORT_TABLE) / sizeof(PumpPort); i++) {
        if (isnormal(vols[i])) {
            error = prepare_for_volume(i, vols[i]);
            if (error != 0) {
                return error;
            }
        }
    }

    return start_timer();
}

int Pump_on(int ch)
{
    gpio_set_level(PUMP_PORT_TABLE[ch].io_pin, 1);
    return 0;
}

int Pump_off(int ch)
{
    gpio_set_level(PUMP_PORT_TABLE[ch].io_pin, 0);
    return 0;
}

int Pump_update_speed(int ch, double speed)
{
    s_pump_status.config.speeds[ch] = speed;
    return save_config();
}

double Pump_get_speed(int ch)
{
    assert(ch > 0 && ch < PUMP_MAX_CHANNELS);
    return s_pump_status.config.speeds[ch];
}

int Pump_is_any_busy()
{
    for (size_t i = 0; i < sizeof(PUMP_PORT_TABLE) / sizeof(PumpPort); i++) {
        if (s_pump_status.channels[i].state != PUMP_STATE_IDLE) {
            return 1;
        }
    }
    return 0;
}

PumpChannelInfo Pump_get_channel_info(int ch)
{
    PumpChannelInfo info = {
        .name = PUMP_PORT_TABLE[ch].name,
        .state = s_pump_status.channels[ch].state,
        .speed = s_pump_status.config.speeds[ch],
    };
    return info;
}

static int prepare_for_volume(int ch, double vol)
{
    // 计算需要执行的时间
    double speed = s_pump_status.config.speeds[ch]; // 假如是 12mL/min
    int duration = (int)round((vol / speed) * (60.0 * 1000.0));
    return prepare_for_duration(ch, duration);
}

static int prepare_for_duration(int ch, int duration)
{
    if (duration <= 0) {
        ESP_LOGE(TAG, "Invalid duration %d for channel %d", duration, ch);
        return PUMP_ERROR_INVALID_VOLUME;
    }

    ESP_LOGI(TAG, "Starting pump %dms for channel %d", duration, ch);
    for (size_t i = 0; i < sizeof(PUMP_PORT_TABLE) / sizeof(PumpPort); i++) {
        PumpChannel* pc = &s_pump_status.channels[ch];
        if (pc->state != PUMP_STATE_IDLE) {
            ESP_LOGE(TAG, "Pump channel %d is busy!", ch);
            return PUMP_ERROR_BUSY;
        }
    }
    PumpChannel* pc = &s_pump_status.channels[ch];
    pc->duration = duration;
    pc->state = PUMP_STATE_WAIT;
    return 0;
}

static int start_timer()
{
    // 设置任务状态
    for (size_t i = 0; i < sizeof(PUMP_PORT_TABLE) / sizeof(PumpPort); i++) {
        PumpChannel* pc = &s_pump_status.channels[i];
        if (pc->state == PUMP_STATE_WAIT) {
            pc->state = PUMP_STATE_BUSY;
            Pump_on(i);
            // 启动定时器
            ESP_ERROR_CHECK(esp_timer_start_once(pc->timer, (uint64_t)pc->duration * 1000ULL));
        }
    }

    ESP_LOGI(TAG, "Timer started...");
    return 0;
}

static void timer_callback(void* params)
{
    int channel_index = (int)params;
    PumpChannel* pc = &s_pump_status.channels[channel_index];
    assert(pc->state == PUMP_STATE_BUSY);
    Pump_off(channel_index);
    pc->state = PUMP_STATE_IDLE;
}

static int save_config()
{
    ESP_LOGI(TAG, "Saving config...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_set_blob(nvs_handle, NVS_PUMP_CONFIG_KEY, &s_pump_status.config, sizeof(s_pump_status.config));
    if (err != ESP_OK) {
        return err;
    }

    err = nvs_commit(nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}

static int load_config()
{
    ESP_LOGI(TAG, "Loading config...");

    nvs_handle_t nvs_handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &nvs_handle);
    if (err != ESP_OK) {
        return err;
    }

    size_t size = sizeof(s_pump_status.config);
    err = nvs_get_blob(nvs_handle, NVS_PUMP_CONFIG_KEY, &s_pump_status.config, &size);
    if (err != ESP_OK) {
        return err;
    }

    nvs_close(nvs_handle);
    return ESP_OK;
}