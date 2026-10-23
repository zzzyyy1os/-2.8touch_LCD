# DeepSeek API 客户端 (HTTPS): 余额查询 + Chat Completions
idf_component_register(
    SRCS "deepseek.c"
    INCLUDE_DIRS "."
    REQUIRES esp_http_client json mbedtls
)
