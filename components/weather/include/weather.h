/**
 * @file weather.h
 * @brief 聚合数据 (juhe.cn) 天气/定位 API 客户端
 *
 * 流程: 先经公网 IP 回显服务拿到出口 IP -> ip/ipNewV3 定位城市
 *       -> simpleWeather/query 按城市查实况+今日预报。
 * 文档: E:\新建文件夹\ (IP地址-根据IP查询地址V3.0 / 天气预报-根据城市查询天气)。
 */
#ifndef WEATHER_H
#define WEATHER_H

#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char city[24];          /* 城市名, 如 "苏州" (不带"市") */
    char cond[32];          /* 实况天气, 如 "多云" / "小雨" */
    int temp;               /* 实况温度 ℃ */
    int hum;                /* 湿度 0-100 */
    int tmin, tmax;         /* 今日最低/最高温 ℃ */
    char wind_dir[24];      /* 风向, 如 "东南风" */
    char wind_pow[12];      /* 风力, 如 "3级" */
} weather_data_t;

typedef struct {
    const char *ip_key;         /* 聚合数据 IP定位接口 APPKEY (必填) */
    const char *weather_key;    /* 聚合数据 天气预报接口 APPKEY (必填) */
    const char *default_city;   /* IP 定位失败兜底城市, NULL -> "杭州" */
} weather_cfg_t;

/** 保存双 key/兜底城市, 只拷贝字符串 */
esp_err_t weather_init(const weather_cfg_t *cfg);

/**
 * @brief 公网 IP -> 城市名 (ipNewV3: City 为空时退 Province, 再退兜底城市)
 * @return ESP_OK 时 city_out 非空; 定位失败也保证写入兜底城市
 */
esp_err_t weather_locate(char *city_out, size_t len);

/** 按城市查实况+今日高低温 (simpleWeather/query, future_num=5) */
esp_err_t weather_query(const char *city, weather_data_t *out);

/** 最近一次失败的简短原因, 供日志/调试 */
const char *weather_last_error(void);

#ifdef __cplusplus
}
#endif

#endif /* WEATHER_H */
