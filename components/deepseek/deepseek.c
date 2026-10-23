/**
 * @file deepseek.c
 * @brief DeepSeek API 客户端实现 (esp_http_client + cJSON)
 */
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "cJSON.h"
#include "deepseek.h"

static const char *TAG = "deepseek";

#define DEEPSEEK_API_BASE   "https://api.deepseek.com"
#define HTTP_TIMEOUT_MS     20000
#define RESP_MAX            8192

static char s_token[128];
static char s_last_err[64];     /* 最近一次失败的简短原因, 供 UI 显示 */

const char *deepseek_last_error(void)
{
    return s_last_err;
}

static void set_err(const char *msg)
{
    strlcpy(s_last_err, msg, sizeof(s_last_err));
}

/* 应答收集缓冲 (esp_http_client 事件回调追加) */
typedef struct {
    char *data;
    int len;
    int cap;
} resp_buf_t;

static esp_err_t http_event(esp_http_client_event_t *evt)
{
    resp_buf_t *rb = (resp_buf_t *)evt->user_data;
    if (evt->event_id == HTTP_EVENT_ON_DATA && evt->data_len > 0 && rb != NULL) {
        if (rb->len + evt->data_len + 1 <= rb->cap) {
            memcpy(rb->data + rb->len, evt->data, evt->data_len);
            rb->len += evt->data_len;
            rb->data[rb->len] = '\0';
        }
    }
    return ESP_OK;
}

/* 同步 HTTP(S) 请求, 应答进 resp; 返回 ESP_OK 且 *out_status==200 才算成功 */
static esp_err_t http_perform(esp_http_client_method_t method, const char *url,
                              const char *body, char *resp, int resp_cap, int *out_status)
{
    resp_buf_t rb = { .data = resp, .len = 0, .cap = resp_cap };
    resp[0] = '\0';

    esp_http_client_config_t cfg = {
        .url = url,
        .method = method,
        .event_handler = http_event,
        .user_data = &rb,
        .timeout_ms = HTTP_TIMEOUT_MS,
        .buffer_size = 2048,
        /* 必须提供 CA 校验: IDF 5.x 不配证书且不开 SKIP_SERVER_CERT_VERIFY 时,
         * esp-tls 直接报 "No server verification option set" 并拒绝连接。
         * 这里挂载内置根证书包 (sdkconfig 已开 CONFIG_MBEDTLS_CERTIFICATE_BUNDLE) */
        .crt_bundle_attach = esp_crt_bundle_attach,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (client == NULL) {
        return ESP_ERR_NO_MEM;
    }

    char auth[192];
    snprintf(auth, sizeof(auth), "Bearer %s", s_token);
    esp_http_client_set_header(client, "Authorization", auth);
    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_header(client, "Accept", "application/json");
    if (method == HTTP_METHOD_POST && body != NULL) {
        esp_http_client_set_post_field(client, body, strlen(body));
    }

    esp_err_t err = esp_http_client_perform(client);
    if (out_status != NULL) {
        *out_status = esp_http_client_get_status_code(client);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "http perform failed: %s", esp_err_to_name(err));
        snprintf(s_last_err, sizeof(s_last_err), "net %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
    return err;
}

esp_err_t deepseek_init(const char *api_token)
{
    if (api_token == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    strlcpy(s_token, api_token, sizeof(s_token));
    ESP_LOGI(TAG, "init, token len=%d", (int)strlen(s_token));
    return ESP_OK;
}

esp_err_t deepseek_get_balance(deepseek_balance_t *out)
{
    if (out == NULL || s_token[0] == '\0') {
        set_err("token not set");
        return ESP_ERR_INVALID_STATE;
    }

    char *resp = malloc(RESP_MAX);
    if (resp == NULL) {
        return ESP_ERR_NO_MEM;
    }

    int status = 0;
    esp_err_t err = http_perform(HTTP_METHOD_GET, DEEPSEEK_API_BASE "/user/balance",
                                 NULL, resp, RESP_MAX, &status);
    if (err == ESP_OK && status != 200) {
        ESP_LOGE(TAG, "balance http status %d: %s", status, resp);
        snprintf(s_last_err, sizeof(s_last_err), "http status %d", status);
        err = ESP_FAIL;
    }
    if (err != ESP_OK) {
        free(resp);
        return err;
    }

    cJSON *root = cJSON_Parse(resp);
    free(resp);
    if (root == NULL) {
        set_err("bad response json");
        return ESP_FAIL;
    }

    memset(out, 0, sizeof(*out));
    cJSON *avail = cJSON_GetObjectItem(root, "is_available");
    out->is_available = cJSON_IsTrue(avail);

    cJSON *infos = cJSON_GetObjectItem(root, "balance_infos");
    if (cJSON_IsArray(infos) && cJSON_GetArraySize(infos) > 0) {
        cJSON *it = cJSON_GetArrayItem(infos, 0);
        const char *s;
        if ((s = cJSON_GetStringValue(cJSON_GetObjectItem(it, "currency"))) != NULL) {
            strlcpy(out->currency, s, sizeof(out->currency));
        }
        if ((s = cJSON_GetStringValue(cJSON_GetObjectItem(it, "total_balance"))) != NULL) {
            strlcpy(out->total_balance, s, sizeof(out->total_balance));
        }
        if ((s = cJSON_GetStringValue(cJSON_GetObjectItem(it, "granted_balance"))) != NULL) {
            strlcpy(out->granted_balance, s, sizeof(out->granted_balance));
        }
        if ((s = cJSON_GetStringValue(cJSON_GetObjectItem(it, "topped_up_balance"))) != NULL) {
            strlcpy(out->topped_up_balance, s, sizeof(out->topped_up_balance));
        }
    }
    cJSON_Delete(root);
    return ESP_OK;
}

esp_err_t deepseek_chat(const char *user_content, char *reply, size_t reply_size)
{
    if (user_content == NULL || reply == NULL || reply_size == 0 || s_token[0] == '\0') {
        return ESP_ERR_INVALID_STATE;
    }
    reply[0] = '\0';

    /* 按 DeepSeek Chat Completions 请求格式组包 */
    cJSON *req = cJSON_CreateObject();
    cJSON *msgs = cJSON_AddArrayToObject(req, "messages");
    cJSON *sys = cJSON_CreateObject();
    cJSON_AddStringToObject(sys, "content", "You are a helpful assistant");
    cJSON_AddStringToObject(sys, "role", "system");
    cJSON_AddItemToArray(msgs, sys);
    cJSON *usr = cJSON_CreateObject();
    cJSON_AddStringToObject(usr, "content", user_content);
    cJSON_AddStringToObject(usr, "role", "user");
    cJSON_AddItemToArray(msgs, usr);

    cJSON_AddStringToObject(req, "model", "deepseek-flash");
    cJSON *thinking = cJSON_AddObjectToObject(req, "thinking");
    cJSON_AddStringToObject(thinking, "type", "enabled");
    cJSON_AddStringToObject(req, "reasoning_effort", "none");
    cJSON_AddNumberToObject(req, "max_tokens", 4096);
    cJSON *rf = cJSON_AddObjectToObject(req, "response_format");
    cJSON_AddStringToObject(rf, "type", "text");
    cJSON_AddNullToObject(req, "stop");
    cJSON_AddBoolToObject(req, "stream", false);
    cJSON_AddNullToObject(req, "stream_options");
    cJSON_AddNumberToObject(req, "temperature", 1);
    cJSON_AddNumberToObject(req, "top_p", 1);
    cJSON_AddNullToObject(req, "tools");
    cJSON_AddStringToObject(req, "tool_choice", "none");
    cJSON_AddBoolToObject(req, "logprobs", false);
    cJSON_AddNullToObject(req, "top_logprobs");

    char *body = cJSON_PrintUnformatted(req);
    cJSON_Delete(req);
    if (body == NULL) {
        return ESP_ERR_NO_MEM;
    }

    char *resp = malloc(RESP_MAX);
    if (resp == NULL) {
        cJSON_free(body);
        return ESP_ERR_NO_MEM;
    }

    int status = 0;
    esp_err_t err = http_perform(HTTP_METHOD_POST, DEEPSEEK_API_BASE "/chat/completions",
                                 body, resp, RESP_MAX, &status);
    cJSON_free(body);
    if (err == ESP_OK && status != 200) {
        ESP_LOGE(TAG, "chat http status %d: %s", status, resp);
        snprintf(s_last_err, sizeof(s_last_err), "http status %d", status);
        err = ESP_FAIL;
    }
    if (err == ESP_OK) {
        cJSON *root = cJSON_Parse(resp);
        cJSON *msg = cJSON_GetObjectItem(
            cJSON_GetArrayItem(cJSON_GetObjectItem(root, "choices"), 0), "message");
        const char *content = cJSON_GetStringValue(cJSON_GetObjectItem(msg, "content"));
        if (content != NULL) {
            strlcpy(reply, content, reply_size);
        } else {
            set_err("bad response json");
            err = ESP_FAIL;
        }
        cJSON_Delete(root);
    }
    free(resp);
    return err;
}
