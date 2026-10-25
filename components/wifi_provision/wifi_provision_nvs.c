/**
 * @file wifi_provision_nvs.c
 * @brief WiFi 凭据 NVS 存取 (namespace "wifi_cfg")
 */
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "wifi_provision_internal.h"

#define NVS_NS      "wifi_cfg"
#define KEY_SSID    "ssid"
#define KEY_PASS    "pass"

esp_err_t wifi_prov_nvs_load(char *ssid, size_t ssid_len, char *pass, size_t pass_len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) {
        ssid[0] = '\0';
        pass[0] = '\0';
        return err;
    }

    size_t sl = ssid_len, pl = pass_len;
    err = nvs_get_str(h, KEY_SSID, ssid, &sl);
    if (err != ESP_OK) {
        ssid[0] = '\0';
    }
    esp_err_t err2 = nvs_get_str(h, KEY_PASS, pass, &pl);
    if (err2 != ESP_OK) {
        pass[0] = '\0';
    }
    nvs_close(h);
    return (err == ESP_OK) ? ESP_OK : err;
}

esp_err_t wifi_prov_nvs_save(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_set_str(h, KEY_SSID, ssid);
    if (err == ESP_OK) {
        err = nvs_set_str(h, KEY_PASS, pass ? pass : "");
    }
    if (err == ESP_OK) {
        err = nvs_commit(h);    /* 必须 commit, 否则掉电即丢 */
    }
    nvs_close(h);
    return err;
}

esp_err_t wifi_prov_nvs_erase(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) {
        return err;
    }
    err = nvs_erase_all(h);
    if (err == ESP_OK) {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return err;
}
