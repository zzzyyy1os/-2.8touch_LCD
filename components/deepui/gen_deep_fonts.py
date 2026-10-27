#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""DEEP 界面 deepui_font___* 的补丁字库 (lv_font_conv)。

AiDoGUI 字库只含设计稿出现过的字; 运行时绑定换成真实字段后会冒出
设计稿没用过的字, 少了就显示方框。给 5 档字号各挂补丁, 主字库
.fallback 指过去 (LVGL 缺字自动回退, 主字体风格不变)。

补丁链: 主字库 -> simhei 补丁 -> arial 顶补丁 (deepui_font_patch_NN_x)
- simhei 是 GBK 字库, 没有 U+00A5 半角 '¥' (只有全角 U+FFE5 '￥'),
  设计稿 BalanceSymbol 用的正是 U+00A5; lv_font_conv 对字库没有的字
  **静默跳过** (stderr 提一句, 照样 exit 0), 补丁自己也缺 → 运行时
  照样方框 (2026-10-27 七修现场)。缺字从 arial 生成顶补丁兜住。
- 生成后按字集反查字形注释, 没有 donor 覆盖的缺字直接报错退出,
  不再让静默跳过得逞。

- 15/18/20/30 (小标签): 补全 ASCII + 运行时用字, 防任意数字/标点缺字
- 60 (大数字): 只补 数字/标点/¥/运行时用字, 省 flash

用法: python -I gen_deep_fonts.py   (port_deepui.py 末尾会自动调用)
"""
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
FONTS_DIR = os.path.join(HERE, "fonts")
sys.path.insert(0, os.path.join(HERE, "..", "aidogui"))
import gen_fonts as gf  # noqa: E402  复用 lv_font_conv 封装与补丁链

# 运行时显示用字: 设计稿静态文本 + deep_ui.c 绑定文本 + 占位符
CJK = "账户余额更新于自动刷新今日消耗额度已用开放平台可充值"
CORE = "0123456789.,:-¥%() " + CJK
# 大数字档 (余额/时钟) 只出数字与符号, 不备全 ASCII
BIG = "0123456789.,:-¥%() "

# simhei 缺、由 arial 顶补丁兜底的字: U+00A5 半角 ¥ (GBK 字库只有 U+FFE5 ￥)
DONOR_FONT = "C:/Windows/Fonts/arial.ttf"
DONOR_CHARS = "¥"

JOBS = [
    (15, CORE, True),
    (18, CORE, True),
    (20, CORE, True),
    (30, CORE, True),
    (60, BIG, False),
]


def main():
    gf.ensure_tool()
    if not os.path.isfile(DONOR_FONT):
        raise SystemExit(f"donor font not found: {DONOR_FONT}")
    os.makedirs(FONTS_DIR, exist_ok=True)
    ascii7 = "".join(chr(c) for c in range(0x20, 0x7F))
    for size, chars, full_ascii in JOBS:
        main_symbol = f"deepui_font___{size}"
        patch_symbol = f"deepui_font_patch_{size}"
        ext_symbol = f"deepui_font_patch_{size}_x"
        patch_path = os.path.join(FONTS_DIR, patch_symbol + ".c")
        ext_path = os.path.join(FONTS_DIR, ext_symbol + ".c")

        # simhei 主补丁 (字集去掉 donor 字, 全缺时 lv_font_conv 会整库报错)
        simhei_chars = "".join(c for c in chars if c not in DONOR_CHARS)
        gf.gen_one(gf.PATCH_FONT, size, simhei_chars,
                   patch_path, patch_symbol, full_ascii=full_ascii)
        # arial 顶补丁兜住 simhei 没有的字, 挂在 simhei 补丁的 .fallback
        gf.gen_one(DONOR_FONT, size, DONOR_CHARS,
                   ext_path, ext_symbol, full_ascii=False)
        gf.link_fallback(patch_path, ext_symbol)
        gf.link_fallback(os.path.join(FONTS_DIR, main_symbol + ".c"), patch_symbol)
        print(f"[{main_symbol}] fallback -> {patch_symbol} -> {ext_symbol}")

        # 守卫: 补丁链必须覆盖整个字集 (lv_font_conv 缺字只警告不报错)
        want = chars + (ascii7 if full_ascii else "")
        miss = set(gf.detect_missing(patch_path, want)) - set(DONOR_CHARS)
        miss |= set(gf.detect_missing(ext_path, DONOR_CHARS))
        if miss:
            raise SystemExit(
                f"[{main_symbol}] 补丁链缺字 {sorted(miss)!r}: "
                f"simhei/arial 都没有, 补 DONOR_CHARS 或改字集")
    print("deepui font patches done")


if __name__ == "__main__":
    main()
