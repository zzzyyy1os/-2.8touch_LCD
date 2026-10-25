# wifi_provision — AP 二维码配网组件

SoftAP + 二维码 + Captive Portal WiFi 配网。触摸屏显示 WiFi 二维码，手机扫码
自动连上设备热点并弹出配网页，填 WiFi 名称/密码后设备存 NVS 重启连网。

## 用法

```c
#include "lvgl_port.h"
#include "wifi_provision_ui_lvgl.h"

void app_main(void)
{
    nvs_flash_init();                       /* 前置 */
    lvgl_port_init(NULL);                   /* LCD + 触摸 + LVGL */
    wifi_provision_ui_lvgl_start(NULL);     /* 显示界面 + 启动配网状态机 */
}
```

- **无凭据**（首次上电）：自动进配网模式，屏显 QR + 热点名 + Reset 按钮
- **有凭据**：STA 直连（默认 30s 超时回落配网模式，凭据保留）
- **重置**：屏幕 "Reset WiFi" 按钮 → 清凭据并无重启刷新配网界面（不重放开机动画）

### 只要网络核心（不用 LVGL）

```c
#include "wifi_provision.h"

wifi_provision_cfg_t cfg = {
    .on_provision_start = my_qr_draw_cb,    /* 拿 qr_payload 自己画二维码 */
    .on_got_ip = my_got_ip_cb,
};
wifi_provision_start(&cfg);
wifi_provision_get_qr_payload(buf, len);    /* WIFI:S:... 串 */
```

无 LVGL 工程：删除 `wifi_provision_ui_lvgl.c`、`ui/` 目录，移除
`idf_component.yml` 的 lvgl 依赖与 CMakeLists 中对应源文件/`PRIV_REQUIRES lvgl lvgl_port`。

## 配网流程

1. 设备开热点 `ESP32-Setup-XXXX`（WPA2，密码默认 `12345678`，进 QR）
2. 屏显二维码（`WIFI:S:<热点名>;T:WPA;P:<密码>;;`）→ 手机扫码自动连上热点
3. Captive Portal（DNS 全劫持 + 系统探测路径 302）自动弹配网页
   （兜底：浏览器打开 `192.168.4.1`）
4. 配网页扫描/填 WiFi → `POST /connect` → 存 NVS → 重启连网
5. 连上后屏显 SSID + IP；失败超时回落配网模式

## 自定义

| 项 | 位置 |
|---|---|
| 热点名/密码 | `wifi_provision_cfg_t.ap_ssid/ap_pass` |
| 配网页样式/文案 | `web_page.h`（UTF-8） |
| 屏幕布局 | `ui/*.xml`（LVGL Pro 编辑后重新生成 `ui/*_gen.c`） |
| 屏上中文 | 已内置子集字体 `ui/lv_font_cn_20.c`（思源黑体，ASCII+链接连网站）；增字用 lv_font_conv 重新生成 |

## 文件

| 文件 | 职责 |
|---|---|
| `wifi_provision.c` | 状态机：NVS→STA/配网、SoftAP、事件、回落 |
| `wifi_provision_nvs.c` | 凭据存取（namespace `wifi_cfg`） |
| `wifi_provision_dns.c` | 劫持 DNS（A→192.168.4.1） |
| `wifi_provision_http.c` | 配网页/扫描 API/`/connect`/captive 302 |
| `wifi_provision_qr.c` | `WIFI:S:...` 二维码内容组装与转义 |
| `wifi_provision_ui_lvgl.c` | LVGL 界面接线（回调→控件、Reset 按钮） |
| `ui/` | LVGL Pro 风格界面（`*.xml` 源 + `*_gen.c` 生成代码） |
