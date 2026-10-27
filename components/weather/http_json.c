/**
 * @file http_json.c
 * @brief 通用 HTTP(S) GET 实现 (esp_http_client + cJSON 应答收集)
 */
#include <stdlib.h>
#include <string.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "http_json.h"

static const char *TAG = "http_json";

#define HTTP_TIMEOUT_MS     15000
#define RESP_INIT_CAP       4096
#define RESP_MAX_CAP        (32 * 1024)

/* 应答收集缓冲 (esp_http_client 事件回调追加, 随应答增长) */
typedef struct {
    char *data;
    int len;
    int cap;
} resp_buf_t;

static esp_err_t http_event(esp_http_client_event_t *evt)
{
    resp_buf_t *rb = (resp_buf_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0 && rb != NULL) {
        if (rb->len + evt->data_len + 1 > rb->cap) {
            int ncap = rb->cap ? rb->cap * 2 : RESP_INIT_CAP;
            while (ncap < rb->len + evt->data_len + 1) {
                ncap *= 2;
            }
            if (ncap > RESP_MAX_CAP) {
                ESP_LOGW(TAG, "response too large, drop tail");
                return ESP_OK;
            }
            char *nd = realloc(rb->data, ncap);
            if (nd == NULL) {
                return ESP_ERR_NO_MEM;
            }
            rb->data = nd;
            rb->cap = ncap;
        }
        memcpy(rb->data + rb->len, evt->data, evt->data_len);
        rb->len += evt->data_len;
        rb->data[rb->len] = '\0';
    }
    return ESP_OK;
}

esp_err_t http_json_get(const char *url, char **out_body, int *out_status)
{
    if (url == NULL || out_body == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    *out_body = NULL;

    resp_buf_t rb = { .data = malloc(RESP_INIT_CAP), .len = 0, .cap = RESP_INIT_CAP };
    if (rb.data == NULL) {
        return ESP_ERR_NO_MEM;
    }
    rb.data[0] = '\0';

    esp_http_client_config_t cfg = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = http_event,
        .user_data = &rb,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .buffer_size = 2048,
        /* 挂载内置根证书包校验服务端 (sdkconfig 已开 CONFIG_MBEDTLS_CERTIFICATE_BUNDLE),
         * 同时兼容 http:// 明文地址 */
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        free(rb.data);
        return ESP_ERR_NO_MEM;
    }
    esp_http_client_set_header(client, "Accept", "application/json, text/plain, */*");

    esp_err_t err = esp_http_client_perform(client);
    if (out_status != NULL) {
        *out_status = esp_http_client_get_status_code(client);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "GET failed: %s (%.60s)", esp_err_to_name(err), url);
    }
    esp_http_client_cleanup(client);

    if (err != ESP_OK) {
        free(rb.data);
        return err;
    }
    if (rb.data == NULL) {          /* 零字节应答也给个空串, 省得调用方判 NULL */
        rb.data = malloc(1);
        if (rb.data == NULL) {
            return ESP_ERR_NO_MEM;
        }
        rb.data[0] = '\0';
    }
    *out_body = rb.data;
    return ESP_OK;
}

void http_json_free(char *body)
{
    free(body);
}
