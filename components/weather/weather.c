/**
 * @file weather.c
 * @brief 聚合数据 API 客户端实现 (HTTP GET + cJSON)
 *
 * 接口:
 *   https://apis.juhe.cn/ip/ipNewV3?ip=<公网IPv4>&key=xxx
 *     -> result.{Country,Province,City,Isp}
 *   https://apis.juhe.cn/simpleWeather/query?city=<utf8urlencoded>&key=xxx&future_num=5
 *     -> result.{city, realtime:{temperature,humidity,info,direct,power}, future[0].temperature:"1/7℃"}
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "cJSON.h"
#include "esp_log.h"
#include "http_json.h"
#include "weather.h"

static const char *TAG = "weather";

#define IP_ECHO_URL         "https://api.ipify.org"     /* 备份回显 (国内常被 GFW RST, 放后备) */
#define IP_ECHO_URL_ALT     "http://ip.3322.net"        /* 主用: 国内纯文本 IPv4 */
#define JUHE_IP_URL         "https://apis.juhe.cn/ip/ipNewV3"
#define JUHE_WEATHER_URL    "https://apis.juhe.cn/simpleWeather/query"

static char s_ip_key[64];
static char s_weather_key[64];
static char s_default_city[24];
static char s_last_err[64];

const char *weather_last_error(void)
{
    return s_last_err;
}

static void set_err(const char *msg)
{
    strlcpy(s_last_err, msg, sizeof(s_last_err));
    ESP_LOGW(TAG, "%s", msg);
}

esp_err_t weather_init(const weather_cfg_t *cfg)
{
    if (cfg == NULL || cfg->ip_key == NULL || cfg->ip_key[0] == '\0' ||
        cfg->weather_key == NULL || cfg->weather_key[0] == '\0') {
        return ESP_ERR_INVALID_ARG;
    }
    if (strncmp(cfg->ip_key, "xxxx", 4) == 0 || strncmp(cfg->weather_key, "xxxx", 4) == 0) {
        /* 防呆: key 还是 example 里的占位符, juhe 必然报错, 数据刷不出来 */
        ESP_LOGE(TAG, "JUHE keys are placeholders! edit main/weather_config.h first");
    }
    strlcpy(s_ip_key, cfg->ip_key, sizeof(s_ip_key));
    strlcpy(s_weather_key, cfg->weather_key, sizeof(s_weather_key));
    strlcpy(s_default_city, (cfg->default_city && cfg->default_city[0]) ?
             cfg->default_city : "杭州", sizeof(s_default_city));
    ESP_LOGI(TAG, "init, default city=%s", s_default_city);
    return ESP_OK;
}

/* UTF-8 百分号编码 (查询参数用); out 需 len >= 3*strlen(in)+1 */
static void url_encode(const char *in, char *out, size_t len)
{
    static const char *hex = "0123456789ABCDEF";
    size_t o = 0;
    for (; *in && o + 4 < len; in++) {
        unsigned char c = (unsigned char)*in;
        if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
            (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
            out[o++] = (char)c;
        } else {
            out[o++] = '%';
            out[o++] = hex[c >> 4];
            out[o++] = hex[c & 0xF];
        }
    }
    out[o] = '\0';
}

/* 剥行政后缀 ("苏州市"->"苏州", "浙江省"->"浙江"): 字库按不带后缀的
 * 地名生成 (gen_fonts.py CITIES 无 "市/省/自治区" 字), 直接显示会缺字 */
static void strip_admin_suffix(char *s)
{
    static const char *suffixes[] = {
        "维吾尔自治区", "回族自治区", "壮族自治区", "特别行政区", "自治区", "省", "市",
    };
    for (size_t i = 0; i < sizeof(suffixes) / sizeof(suffixes[0]); i++) {
        size_t n = strlen(suffixes[i]);
        size_t l = strlen(s);
        if (l > n && strcmp(s + l - n, suffixes[i]) == 0) {
            s[l - n] = '\0';
            return;
        }
    }
}

/* 像点分十进制 IPv4 (4 段 0-255); 防止把 HTML 错误页当 IP 传给定位接口 */
static bool looks_like_ipv4(const char *s)
{
    int dots = 0, digits = 0, seg = 0;
    for (; *s; s++) {
        if (*s >= '0' && *s <= '9') {
            seg = seg * 10 + (*s - '0');
            if (++digits > 3 || seg > 255) {
                return false;
            }
        } else if (*s == '.') {
            if (digits == 0 || ++dots > 3) {
                return false;
            }
            seg = 0;
            digits = 0;
        } else {
            return false;
        }
    }
    return dots == 3 && digits > 0;
}

/* 公网出口 IPv4 (设备在 NAT 后, 定位接口要的是公网 IP); 主备双源 */
static esp_err_t get_public_ip(char *ip_out, size_t len)
{
    static const char *echo_urls[] = { IP_ECHO_URL_ALT, IP_ECHO_URL };
    for (size_t i = 0; i < sizeof(echo_urls) / sizeof(echo_urls[0]); i++) {
        char *body = NULL;
        esp_err_t err = http_json_get(echo_urls[i], &body, NULL);
        if (err == ESP_OK && body != NULL) {
            /* 回显服务返回纯文本 IP, 去掉可能的换行 */
            strlcpy(ip_out, body, len);
            http_json_free(body);
            char *p = strpbrk(ip_out, "\r\n ");
            if (p) {
                *p = '\0';
            }
            if (looks_like_ipv4(ip_out)) {
                return ESP_OK;
            }
        } else {
            http_json_free(body);
        }
    }
    set_err("get public ip failed");
    return ESP_FAIL;
}

esp_err_t weather_locate(char *city_out, size_t len)
{
    if (city_out == NULL || len == 0) {
        return ESP_ERR_INVALID_ARG;
    }

    char ip[24];
    if (get_public_ip(ip, sizeof(ip)) == ESP_OK) {
        char url[256];
        snprintf(url, sizeof(url), "%s?ip=%s&key=%s", JUHE_IP_URL, ip, s_ip_key);
        char *body = NULL;
        int status = 0;
        if (http_json_get(url, &body, &status) == ESP_OK && body != NULL && status == 200) {
            cJSON *root = cJSON_Parse(body);
            cJSON *err_code = cJSON_GetObjectItem(root, "error_code");
            cJSON *result = cJSON_GetObjectItem(root, "result");
            if (cJSON_IsNumber(err_code) && err_code->valueint == 0 && cJSON_IsObject(result)) {
                cJSON *city = cJSON_GetObjectItem(result, "City");
                cJSON *prov = cJSON_GetObjectItem(result, "Province");
                const char *name = (cJSON_IsString(city) && city->valuestring[0]) ? city->valuestring :
                                   (cJSON_IsString(prov) ? prov->valuestring : NULL);
                if (name && name[0]) {
                    strlcpy(city_out, name, len);
                    strip_admin_suffix(city_out);
                    cJSON_Delete(root);
                    http_json_free(body);
                    ESP_LOGI(TAG, "locate %s -> %s", ip, city_out);
                    return ESP_OK;
                }
            }
            cJSON_Delete(root);
        }
        http_json_free(body);
        set_err("ip locate no result");
    }

    /* 兜底: 配置的城市 */
    strlcpy(city_out, s_default_city, len);
    strip_admin_suffix(city_out);
    return ESP_ERR_NOT_FOUND;
}

/* "1/7℃" -> tmin=1, tmax=7; 解析失败返回 false */
static bool parse_temp_range(const char *s, int *tmin, int *tmax)
{
    if (s == NULL) {
        return false;
    }
    return sscanf(s, "%d/%d", tmin, tmax) == 2;
}

static void get_str(const cJSON *obj, const char *name, char *out, size_t len)
{
    const cJSON *it = cJSON_GetObjectItem(obj, name);
    if (cJSON_IsString(it)) {
        strlcpy(out, it->valuestring, len);
    }
}

esp_err_t weather_query(const char *city, weather_data_t *out)
{
    if (city == NULL || city[0] == '\0' || out == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    char enc[96];
    char url[320];
    url_encode(city, enc, sizeof(enc));
    snprintf(url, sizeof(url), "%s?city=%s&key=%s&future_num=5",
             JUHE_WEATHER_URL, enc, s_weather_key);

    char *body = NULL;
    int status = 0;
    esp_err_t err = http_json_get(url, &body, &status);
    if (err != ESP_OK || body == NULL) {
        set_err("weather request failed");
        http_json_free(body);
        return ESP_FAIL;
    }

    memset(out, 0, sizeof(*out));
    cJSON *root = cJSON_Parse(body);
    http_json_free(body);
    if (root == NULL) {
        set_err("weather json parse failed");
        return ESP_FAIL;
    }

    cJSON *err_code = cJSON_GetObjectItem(root, "error_code");
    if (!cJSON_IsNumber(err_code) || err_code->valueint != 0) {
        snprintf(s_last_err, sizeof(s_last_err), "api err %.20s",
                 cJSON_IsNumber(err_code) ? "code" : "format");
        if (cJSON_IsNumber(err_code)) {
            snprintf(s_last_err, sizeof(s_last_err), "api error_code=%d", err_code->valueint);
        }
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    cJSON *result = cJSON_GetObjectItem(root, "result");
    if (!cJSON_IsObject(result)) {
        set_err("weather result missing");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    get_str(result, "city", out->city, sizeof(out->city));

    cJSON *rt = cJSON_GetObjectItem(result, "realtime");
    if (cJSON_IsObject(rt)) {
        char buf[32] = "";      /* 字段缺失时 get_str 不写, 空串让 atoi 得 0 */
        get_str(rt, "info", out->cond, sizeof(out->cond));
        get_str(rt, "direct", out->wind_dir, sizeof(out->wind_dir));
        get_str(rt, "power", out->wind_pow, sizeof(out->wind_pow));

        get_str(rt, "temperature", buf, sizeof(buf));
        out->temp = atoi(buf);
        get_str(rt, "humidity", buf, sizeof(buf));
        out->hum = atoi(buf);
        if (out->hum < 0) out->hum = 0;
        if (out->hum > 100) out->hum = 100;
    }

    /* future[0].temperature 形如 "1/7℃"; 顺带兜底实况缺失字段 */
    cJSON *future = cJSON_GetObjectItem(result, "future");
    if (cJSON_IsArray(future) && cJSON_GetArraySize(future) > 0) {
        cJSON *f0 = cJSON_GetArrayItem(future, 0);
        char buf[32] = "";
        get_str(f0, "temperature", buf, sizeof(buf));
        parse_temp_range(buf, &out->tmin, &out->tmax);
        if (out->cond[0] == '\0') {
            get_str(f0, "weather", out->cond, sizeof(out->cond));
        }
    }
    cJSON_Delete(root);

    if (out->city[0] == '\0') {
        strlcpy(out->city, city, sizeof(out->city));
    }
    strip_admin_suffix(out->city);
    ESP_LOGI(TAG, "query %s: %s %d°C hum=%d %d~%d %s%s",
             out->city, out->cond, out->temp, out->hum,
             out->tmin, out->tmax, out->wind_dir, out->wind_pow);
    return ESP_OK;
}
