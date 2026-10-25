/**
 * @file wifi_provision_internal.h
 * @brief 组件内部接口 (NVS / QR / DNS / HTTP)
 */
#ifndef WIFI_PROVISION_INTERNAL_H
#define WIFI_PROVISION_INTERNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"
#include "wifi_provision.h"     /* WIFI_PROV_* 容量宏 */

/* ---- wifi_provision_nvs.c ---- */
esp_err_t wifi_prov_nvs_load(char *ssid, size_t ssid_len, char *pass, size_t pass_len);
esp_err_t wifi_prov_nvs_save(const char *ssid, const char *pass);
esp_err_t wifi_prov_nvs_erase(void);

/* ---- wifi_provision_qr.c ---- */
esp_err_t wifi_prov_qr_build(char *buf, size_t len,
                             const char *ssid, const char *pass, bool is_open);

/* ---- wifi_provision_dns.c ---- */
esp_err_t wifi_prov_dns_start(void);
void wifi_prov_dns_stop(void);

/* ---- wifi_provision_http.c ---- */
esp_err_t wifi_prov_http_start(void);
void wifi_prov_http_stop(void);

#endif /* WIFI_PROVISION_INTERNAL_H */
