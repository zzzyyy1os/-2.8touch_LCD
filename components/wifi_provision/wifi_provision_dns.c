/**
 * @file wifi_provision_dns.c
 * @brief Captive Portal 劫持 DNS (UDP 53, 所有 A 记录答 192.168.4.1)
 *
 * 手机连上热点后域名解析全部指向本机, 系统连通性探测才会弹出配网页。
 * recvfrom 带 500ms 超时轮询退出标志, 停止时不依赖 close 打断阻塞。
 */
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "wifi_provision_internal.h"

static const char *TAG = "prov_dns";

#define DNS_PORT        53
#define DNS_BUF_MAX     512
#define AP_IP_ADDR      "192.168.4.1"

static volatile bool s_dns_run;
static TaskHandle_t s_dns_task;
static int s_fd = -1;

/* 单包应答: 请求头+Question 原样回显, 追加一条 A 记录 */
static int dns_build_reply(const uint8_t *req, int req_len, uint8_t *rep, int rep_max)
{
    if (req_len < 12 || req_len > rep_max) {
        return -1;
    }
    /* 定位 Question 结尾: 域名标签序列 + \0 + QTYPE(2) + QCLASS(2) */
    int q_end = 12;
    while (q_end < req_len && req[q_end] != 0) {
        if (req[q_end] > 63) {
            return -1;                  /* 压缩指针等异常包直接丢弃 */
        }
        q_end += req[q_end] + 1;
    }
    if (q_end + 5 > req_len) {
        return -1;
    }
    q_end += 5;                         /* 跳过 \0 + QTYPE + QCLASS */

    uint16_t qtype = (uint16_t)((req[q_end - 4] << 8) | req[q_end - 3]);

    memcpy(rep, req, q_end);
    rep[2] = 0x81;                      /* QR=1, RD=1, RCODE=0 */
    rep[3] = 0x80;
    rep[6] = rep[7] = 0;                /* ANCOUNT */
    rep[8] = rep[9] = 0;                /* NSCOUNT */
    rep[10] = rep[11] = 0;              /* ARCOUNT */

    if (qtype != 1) {                   /* 非 A 记录 (如 AAAA): 空应答 */
        return q_end;
    }
    if (q_end + 16 > rep_max) {
        return -1;
    }

    rep[7] = 1;                         /* ANCOUNT = 1 */
    int a = q_end;
    rep[a++] = 0xC0; rep[a++] = 0x0C;   /* 域名指针 -> Question */
    rep[a++] = 0x00; rep[a++] = 0x01;   /* TYPE A */
    rep[a++] = 0x00; rep[a++] = 0x01;   /* CLASS IN */
    rep[a++] = 0x00; rep[a++] = 0x00;
    rep[a++] = 0x00; rep[a++] = 30;     /* TTL 30s */
    rep[a++] = 0x00; rep[a++] = 0x04;   /* RDLENGTH */
    struct in_addr ip;
    inet_pton(AF_INET, AP_IP_ADDR, &ip);
    memcpy(&rep[a], &ip.s_addr, 4);
    a += 4;
    return a;
}

static void dns_task(void *arg)
{
    uint8_t *buf = malloc(DNS_BUF_MAX * 2);
    if (buf == NULL) {
        s_dns_task = NULL;
        vTaskDelete(NULL);
        return;
    }
    uint8_t *req = buf;
    uint8_t *rep = buf + DNS_BUF_MAX;

    while (s_dns_run) {
        struct sockaddr_in src;
        socklen_t slen = sizeof(src);
        int n = recvfrom(s_fd, req, DNS_BUF_MAX, 0, (struct sockaddr *)&src, &slen);
        if (n < 0) {
            continue;                   /* 超时, 回到循环检查退出标志 */
        }
        int rlen = dns_build_reply(req, n, rep, DNS_BUF_MAX);
        if (rlen > 0) {
            sendto(s_fd, rep, rlen, 0, (struct sockaddr *)&src, slen);
        }
    }
    free(buf);
    s_dns_task = NULL;
    vTaskDelete(NULL);
}

esp_err_t wifi_prov_dns_start(void)
{
    if (s_dns_task || s_fd >= 0) {
        return ESP_ERR_INVALID_STATE;
    }

    int fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_IP);
    if (fd < 0) {
        return ESP_FAIL;
    }
    int reuse = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    struct timeval tv = { .tv_sec = 0, .tv_usec = 500 * 1000 };
    setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(DNS_PORT),
        .sin_addr.s_addr = htonl(INADDR_ANY),
    };
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return ESP_FAIL;
    }

    s_fd = fd;
    s_dns_run = true;
    if (xTaskCreate(dns_task, "prov_dns", 3072, NULL, 2, &s_dns_task) != pdPASS) {
        s_dns_run = false;
        close(fd);
        s_fd = -1;
        return ESP_ERR_NO_MEM;
    }
    ESP_LOGI(TAG, "captive DNS started on :%d -> %s", DNS_PORT, AP_IP_ADDR);
    return ESP_OK;
}

void wifi_prov_dns_stop(void)
{
    s_dns_run = false;
    for (int i = 0; i < 20 && s_dns_task; i++) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }
    if (s_fd >= 0) {
        close(s_fd);
        s_fd = -1;
    }
}
