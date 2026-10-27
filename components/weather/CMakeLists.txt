# 聚合数据 (juhe.cn) 天气/定位 API 客户端 (HTTP GET + cJSON)
idf_component_register(
    SRCS "http_json.c" "weather.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_http_client json mbedtls
)
