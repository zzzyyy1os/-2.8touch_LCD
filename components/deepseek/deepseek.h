/**
 * @file deepseek.h
 * @brief DeepSeek API 客户端 (HTTPS)
 *
 *   deepseek_init()        设置 API token
 *   deepseek_get_balance() 余额查询 (GET /user/balance)
 *   deepseek_chat()        Chat Completions (POST /chat/completions)
 *
 * 内部走 esp_http_client + cJSON, 调用为同步阻塞式(在线程里用)。
 */
#ifndef DEEPSEEK_H
#define DEEPSEEK_H

#include <stddef.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 账户余额信息 */
typedef struct {
    bool is_available;          /* 是否可继续调用 */
    char currency[8];           /* CNY / USD */
    char total_balance[32];     /* 总余额 */
    char granted_balance[32];   /* 赠送余额 */
    char topped_up_balance[32]; /* 充值余额 */
} deepseek_balance_t;

/**
 * @brief 初始化 (保存 token), 可重复调用以更换 token
 */
esp_err_t deepseek_init(const char *api_token);

/**
 * @brief 最近一次 API 失败的简短原因 (如 "net ESP_ERR_..." / "http status 401")
 */
const char *deepseek_last_error(void);

/**
 * @brief 查询账户余额 (GET https://api.deepseek.com/user/balance)
 */
esp_err_t deepseek_get_balance(deepseek_balance_t *out);

/**
 * @brief Chat Completions 一问一答 (POST /chat/completions)
 * @param user_content  用户提问
 * @param reply         输出: 模型回答 (截断到 reply_size-1)
 */
esp_err_t deepseek_chat(const char *user_content, char *reply, size_t reply_size);

#ifdef __cplusplus
}
#endif

#endif /* DEEPSEEK_H */
