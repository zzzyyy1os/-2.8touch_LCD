/**
 * @file http_json.h
 * @brief 通用 HTTP(S) GET 小工具, 应答收进 malloc 缓冲 (NUL 结尾)
 *
 * 复用 deepseek 组件的 esp_http_client + esp_crt_bundle 模式,
 * 缓冲随应答增长, 不限 8KB (天气 5 日预报 JSON 较大)。
 */
#ifndef HTTP_JSON_H
#define HTTP_JSON_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief 同步 HTTP(S) GET, 应答体存 *out_body (malloc, 调用方 http_json_free)
 * @param out_status 可 NULL; HTTP 状态码 (200 才算业务成功)
 * @return ESP_OK 表示传输完成 (业务对错看状态码/JSON 内容)
 */
esp_err_t http_json_get(const char *url, char **out_body, int *out_status);

void http_json_free(char *body);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_JSON_H */
