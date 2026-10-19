# lcd_touch 组件

从厂商例程 **P024C135**（`ESP32-LCD-IDF`，原目标 ESP32-C3 / IDF 5.4.2）移植到 **ESP32-S3** 的 LCD + 触摸驱动。
显示为寄存器直写式裸驱动（不依赖 `esp_lcd` / LVGL），触摸为 FT6236 电容触摸（软件 I2C）。

## 硬件

| 项目 | 说明 |
|---|---|
| LCD | ILI9341 兼容控制器，240×320，RGB565，SPI 4 线，10 MHz |
| 触摸 | FT6236（I2C 地址 0x38），软件模拟 I2C，轮询读取（INT 脚未接中断） |

## 引脚分配（ESP32-S3，本组件当前配置）

| 信号 | GPIO | 说明 |
|---|---|---|
| LCD_SCK | 12 | SPI 时钟 |
| LCD_MOSI | 11 | SPI 数据 |
| LCD_CS | 10 | 片选（硬件 CS） |
| LCD_DC | 9 | 数据/命令 |
| LCD_RES | 4 | 复位 |
| LCD_BLK | 5 | 背光，高电平点亮 |
| TOUCH_SDA | 15 | 软 I2C SDA（需外部上拉） |
| TOUCH_SCL | 16 | 软 I2C SCL（需外部上拉） |
| TOUCH_RST | 17 | 触摸复位 |
| TOUCH_INT | 18 | 触摸中断（当前代码未使用） |

引脚集中定义在 `Inc/lcd_init.h`（LCD）与 `Inc/ft6236.h`（触摸），改动引脚只需编辑这两个文件。
选脚已避开 Flash（26-32）、USB-JTAG（19/20）、UART0（43/44）、strapping（0/3/45/46）及八线 PSRAM（35-37）。

## API 速查

初始化：`LCD_Init()`、`LCD_BLK_Set()` / `LCD_BLK_Clr()`、`FT6236_Init()`

绘图（`lcd.h`）：
- 区域/几何：`LCD_Fill`、`LCD_DrawPoint`、`LCD_DrawLine`、`LCD_DrawRectangle`、`Draw_Circle`、`LCD_FillCircle`、`LCD_DrawThickLine`
- 文字：`LCD_ShowChar`、`LCD_ShowString`、`LCD_ShowChinese`（12/16/24/32）、`LCD_ShowIntNum`、`LCD_ShowFloatNum1`
- 图片：`LCD_ShowPicture(x, y, 宽, 高, 数据)` —— 数据为 RGB565 逐行像素
- 颜色宏：`WHITE/BLACK/RED/GREEN/BLUE/YELLOW/CYAN/MAGENTA/...`，或 `RGB(r,g,b)` 自配

触摸（`ft6236.h`）：
- `FT6236_Get_Touch_Data()` 后读全局 `FT6236_Instance`（`X_Pos/Y_Pos/Touch_Count/Touch_Event`）
- 校准宏（`Src/ft6236.c` 顶部）：`TOUCH_OFFSET_X/Y`、`REVERSE_X/Y`（现为 1/1，触摸相对显示旋转 180°）、`TOUCH_MAX_X/Y`（镜像基准，取屏幕宽/高-1：239/319）
- 其它：`FT6236_Get_ChipID`、`FT6236_Set_Threshold`、`FT6236_Sleep` 等

## 使用示例

```c
#include "lcd.h"
#include "lcd_init.h"
#include "ft6236.h"

void app_lcd_init(void)          /* 建议在独立任务中调用 */
{
    LCD_Init();
    FT6236_Init();
    LCD_Fill(0, 0, LCD_W - 1, LCD_H - 1, BLACK);
    LCD_BLK_Set();
    LCD_ShowString(10, 10, (const uint8_t *)"Hello", WHITE, BLACK, 16, 0);

    FT6236_Get_Touch_Data();
    if (FT6236_Instance.Touch_Count > 0) {
        /* 使用 FT6236_Instance.X_Pos / Y_Pos */
    }
}
```

## 自检

`lcd_touch_selftest_start()`（`Inc/lcd_touch_selftest.h`）创建独立任务：
五色刷屏 → 触摸画点（串口打印坐标）。已在 `espnow_example_main.c` 的
`app_main` 末尾调用，验证完成后可删除该调用与 `Src/lcd_touch_selftest.c`。

## 移植注意事项

1. **编码**：`lcd_init.c/h`、`lcd.c/h`、`ft6236.c/h`、`lcdfont.h` 保持 GBK 原编码（字节级复制，未转 UTF-8）。
   `LCD_ShowChinese` 按 **GB2312/GBK 双字节内码** 匹配 `lcdfont.h` 字库，因此传入的中文字符串
   必须是 GBK 编码字面量（源文件存为 GBK，或用 `"\\xB2\\xE3"` 之类十六进制转义）。
   UTF-8 源文件里的中文字面量与字库不匹配，会显示空白。字库表仅含少量演示汉字，实用需自行补字模。
2. **字节序**：`LCD_Fill` 内部对颜色做了高低字节交换，`LCD_WR_DATA` 是高字节先发——两条路径已自洽，
   改驱动时勿只改其一。
3. **`LCD_ShowPicture`**：直接发送缓冲区原始字节。Image2LCD 导出数组若带 8 字节文件头，需跳过头
   （数据指针 +8）再传入，否则头字节会被刷成像素。
4. **软 I2C**：`iic_hal.c` 用 GPIO 位操作模拟 I2C（2 µs 位延时），SDA/SCL 未使能内部上拉，
   需外部上拉电阻。INT 脚未接中断，触摸靠轮询（≥20 ms 一次足够）。
5. **Flash 预算**：本组件字库约 94 KB 常量，无图片资源；厂商 `pic.h`（2.1 MB）与示例
   `lcd_demo.c` 未移植（超出本次范围），2MB Flash 可容纳。
6. **sdkconfig**：无需额外配置项。`MINIMAL_BUILD ON` 下 main 已显式 `PRIV_REQUIRES lcd_touch`。
