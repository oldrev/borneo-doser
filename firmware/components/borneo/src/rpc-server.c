#include <esp_event.h>
#include <esp_log.h>
#include <esp_system.h>
#include <esp_wifi.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <string.h>
#include <sys/param.h>

#include <lwip/err.h>
#include <lwip/netdb.h>
#include <lwip/sockets.h>
#include <lwip/sys.h>

#include "borneo/common.h"
#include "borneo/device-config.h"
#include "borneo/rpc-server.h"

// 这里只能支持一个连接，需要改成 select() 的支持两个连接
// 还有超时，五分钟不传输数据需要关闭连接

#define SEND_TIMEOUT    5
#define RECV_TIMEOUT    300

const char* TAG = "SERVER";

#define MAX_TX_BUF_SIZE (1024 * 8)
#define MAX_RX_BUF_SIZE (1024 * 8)

typedef struct {
    RpcRequestHandler* request_handler;
    uint8_t tx_buf[MAX_TX_BUF_SIZE];
    uint8_t rx_buf[MAX_RX_BUF_SIZE];
    TaskHandle_t thread;
    bool is_closed;
} RpcServerContext;

static RpcServerContext s_context;

static void tcp_server_task(void* pvParameters);
static int handle_buffer(uint8_t* rx_buf, size_t* recv_size, size_t* size_to_send);

int RpcServer_init(RpcRequestHandler* request_handler)
{
    s_context.request_handler = request_handler;
    s_context.thread = NULL;
    s_context.is_closed = false;
    return 0;
}

int RpcServer_start()
{
    assert(!s_context.is_closed);
    xTaskCreate(tcp_server_task, "rpc-server", 1024 * 8, NULL, tskIDLE_PRIORITY + 5, &s_context.thread);
    return 0;
}

int RpcServer_stop()
{
    assert(!s_context.is_closed);
    assert(s_context.thread != NULL);
    return -1;
}

int RpcServer_close()
{
    assert(!s_context.is_closed);
    assert(s_context.thread == NULL);
    s_context.is_closed = true;
    // TODO
    return -1;
}

static void tcp_server_task(void* pvParameters)
{
    char addr_str[128];
    int addr_family;
    int ip_protocol;

    struct sockaddr_in dest_addr;
    dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(BORNEO_DEVICE_TCP_PORT);
    addr_family = AF_INET;
    ip_protocol = IPPROTO_IP;
    inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

    int listen_sock = socket(addr_family, SOCK_STREAM, ip_protocol);
    if (listen_sock < 0) {
        ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
        return;
    }
    ESP_LOGI(TAG, "Socket created");


    int err = bind(listen_sock, (struct sockaddr*)&dest_addr, sizeof(dest_addr));
    if (err != 0) {
        ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        goto __TASK_EXIT;
    }
    ESP_LOGI(TAG, "Socket bound, port %d", BORNEO_DEVICE_TCP_PORT);

    err = listen(listen_sock, 1);
    if (err != 0) {
        ESP_LOGE(TAG, "Error occurred during listen: errno %d", errno);
        goto __TASK_EXIT;
    }
    ESP_LOGI(TAG, "Socket listening");

    while (1) {
        struct sockaddr_in6 source_addr; // Large enough for both IPv4 or IPv6
        uint addr_len = sizeof(source_addr);
        int client_sock = accept(listen_sock, (struct sockaddr*)&source_addr, &addr_len);
        if (client_sock < 0) {
            ESP_LOGE(TAG, "Unable to accept connection: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket accepted");

        // 设置超时时间
        struct timeval send_timeout = { SEND_TIMEOUT, 0 }; // 发送超时
        struct timeval recv_timeout = { RECV_TIMEOUT, 0 }; // 接收超时
        err = setsockopt(client_sock, SOL_SOCKET, SO_SNDTIMEO, &send_timeout, sizeof(send_timeout));
        err = setsockopt(client_sock, SOL_SOCKET, SO_RCVTIMEO, &recv_timeout, sizeof(recv_timeout));

        size_t rxbuf_size = 0;
        memset(s_context.rx_buf, 0, sizeof(MAX_RX_BUF_SIZE));
        while (1) {
            ssize_t received_size = recv(client_sock, s_context.rx_buf + rxbuf_size, MAX_RX_BUF_SIZE - rxbuf_size, 0);
            // Error occurred during receiving
            if (received_size < 0) {
                ESP_LOGE(TAG, "recv() failed: errno %d", errno);
                break;
            } else if (received_size == 0) { // 连接正常关闭
                break;
            }
            // Data received
            else {
                // Get the sender's ip address as string
                if (source_addr.sin6_family == PF_INET) {
                    inet_ntoa_r(((struct sockaddr_in*)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                } else if (source_addr.sin6_family == PF_INET6) {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                // 收到 \0 我们才认为是一个完整的请求
                rxbuf_size += received_size;
                size_t size_to_send = 0;
                ESP_ERROR_CHECK(handle_buffer(s_context.rx_buf, &rxbuf_size, &size_to_send));
                if (size_to_send > 0) {
                    err = send(client_sock, s_context.tx_buf, size_to_send, 0);
                    if (err < 0) {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
                }
            }
        }

        if (client_sock != -1) {
            shutdown(client_sock, 0);
            close(client_sock);
        }
    }

__TASK_EXIT:

    close(listen_sock);
    vTaskDelete(NULL);
}

static int handle_buffer(uint8_t* rx_buf, size_t* recv_size, size_t* size_to_send)
{
    *size_to_send = 0;
    uint8_t* tx_buf = s_context.tx_buf;
    while (*recv_size > 0) {
        // 查找 '\0' 第一次出现的位置
        uint8_t* found = (uint8_t*)memchr(rx_buf, 0, *recv_size);
        if (found == NULL) {
            // 如果数据里没有 '\0'，说明连一个完整的 JSON-RPC 请求都没接收完
            // 所以不做处理，等着下次继续接收再说
            break;
        }
        // 收到的包里包含 '\0'，则作为 RPC 请求解析
        // 如果对方一次发送里包含 '\0' 分割的多个请求，也可以处理，
        // 多次调用 RPC 处理函数完了一次性发送给客户端
        size_t request_buf_size = found - rx_buf + 1; // 需要包含结尾 \0
        size_t tx_size = MAX_TX_BUF_SIZE - *size_to_send;
        // 下面将提供的缓冲区解析 JSON-RPC 然后执行
        // 每次执行结果附加到 tx_buf 里，最后一次发送
        // 下面的 JSON-RPC 响应结果写道 tx_buf 里的数据最后不能包含 '\0'
        int ret = s_context.request_handler->handle_rpc(rx_buf, request_buf_size, tx_buf, &tx_size);
        if (ret != 0) {
            return ret;
        }
        tx_buf[tx_size] = '\0';
        tx_size++; // 留出 \0
        *size_to_send += tx_size;
        tx_buf += tx_size;
        assert(*size_to_send <= MAX_TX_BUF_SIZE);
        *recv_size -= request_buf_size;
        if (*recv_size > 0) {
            // 如果我们已经处理了请求，那么将后面收到的可能完整也可能不完整的请求数据往前移动到缓冲区头
            // 下次循环再处理
            memmove(rx_buf, found + 1, *recv_size);
        }
        // 如果收到的缓冲区里有多个请求，需要循环处理后面的，这样才能多个 JSON-RPC
        // 一次响应
        rx_buf += request_buf_size;
    }
    return 0;
}
