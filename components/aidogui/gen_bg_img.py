#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把 AiDoGUI 导出的背景图 C 文件 (ARGB8888) 转成 RGB565 的 C 数组。

原始导出 ui_img_t04f3a84e8f9ca6da16_jpg.c 是 658x987 ARGB8888 原始像素
(~2.6MB 数据), 超出 2MB flash 预算, 必须转小后再进固件。输出尺寸跟随
ui_screen1.c 里 Image1 控件大小 (当前 339x254, 位置 -9,-5 四周出血);
LVGL 图片按原始尺寸绘制, 转成控件大小可与设计器"填满控件"预览一致。
符号名保持不变, ui_screen1.c / ui_images.h 无需改动。

用法:
    python -I gen_bg_img.py <src.c> <dst.c> [--mode fill|cover|top] [--preview out.png]

    fill  : 整图拉伸到 320x240 (与设计器"图片填满控件"预览一致)
    cover : 等比放大后居中裁剪 (不拉伸变形)
    top   : 左上角原尺寸裁剪 (LVGL 不缩放时的真实显示区域)
"""
import re
import struct
import sys
import zlib

SRC_W, SRC_H = 658, 987
DST_W, DST_H = 339, 254          # 跟随 ui_screen1.c 里 Image1 控件大小


def parse_map(path):
    """从导出的 .c 文件里抽出 map 数组字节 (B,G,R,A 顺序, LVGL ARGB8888)。"""
    with open(path, "r", encoding="utf-8", errors="ignore") as f:
        text = f.read()
    m = re.search(r"ui_img_t04f3a84e8f9ca6da16_jpg_map\[\]\s*=\s*\{", text)
    if not m:
        raise SystemExit("map array not found in " + path)
    end = text.find("};", m.end())
    if end < 0:
        raise SystemExit("map array end not found")
    blob = bytes(int(h, 16) for h in re.findall(r"0x([0-9a-fA-F]{2})", text[m.end():end]))
    expect = SRC_W * SRC_H * 4
    if len(blob) != expect:
        raise SystemExit(f"unexpected map size {len(blob)} (want {expect})")
    return blob


def sample_bilinear(blob, x, y):
    """取 (x, y) 浮点坐标的像素, 双线性插值, 返回 (r, g, b)。"""
    x0, y0 = int(x), int(y)
    x1, y0 = min(x0 + 1, SRC_W - 1), y0
    y1 = min(y0 + 1, SRC_H - 1)
    fx, fy = x - x0, y - y0
    out = []
    for c in (2, 1, 0):  # r, g, b (map 里是 B,G,R,A)
        p00 = blob[(y0 * SRC_W + x0) * 4 + c]
        p10 = blob[(y0 * SRC_W + x1) * 4 + c]
        p01 = blob[(y1 * SRC_W + x0) * 4 + c]
        p11 = blob[(y1 * SRC_W + x1) * 4 + c]
        top = p00 + (p10 - p00) * fx
        bot = p01 + (p11 - p01) * fx
        out.append(int(round(top + (bot - top) * fy)))
    return out  # r, g, b


def resample(blob, mode):
    """生成 320x240 的 (r, g, b) 像素列表。"""
    pixels = []
    if mode == "fill":
        sx, sy = SRC_W / DST_W, SRC_H / DST_H
        for dy in range(DST_H):
            for dx in range(DST_W):
                pixels.append(sample_bilinear(blob, (dx + 0.5) * sx - 0.5, (dy + 0.5) * sy - 0.5))
    elif mode == "cover":
        scale = max(DST_W / SRC_W, DST_H / SRC_H)      # 等比放大到盖满
        ox = (SRC_W * scale - DST_W) / 2 / scale        # 源图上要裁掉的边距
        oy = (SRC_H * scale - DST_H) / 2 / scale
        for dy in range(DST_H):
            for dx in range(DST_W):
                pixels.append(sample_bilinear(blob, ox + (dx + 0.5) / scale - 0.5,
                                              oy + (dy + 0.5) / scale - 0.5))
    elif mode == "top":
        for dy in range(DST_H):
            for dx in range(DST_W):
                pixels.append(sample_bilinear(blob, dx, dy))
    else:
        raise SystemExit("unknown mode " + mode)
    return pixels


def rgb565(r, g, b):
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def write_png(path, pixels):
    """无依赖的 PNG 输出 (仅预览用)。"""
    raw = b"".join(
        b"\x00" + bytes(v for px in pixels[y * DST_W:(y + 1) * DST_W] for v in px)
        for y in range(DST_H)
    )

    def chunk(tag, data):
        body = tag + data
        return struct.pack(">I", len(data)) + body + struct.pack(">I", zlib.crc32(body))

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n")
        f.write(chunk(b"IHDR", struct.pack(">IIBBBBB", DST_W, DST_H, 8, 2, 0, 0, 0)))
        f.write(chunk(b"IDAT", zlib.compress(raw, 6)))
        f.write(chunk(b"IEND", b""))


def write_c(path, pixels):
    lines = []
    for i in range(0, len(pixels), 8):
        chunk = pixels[i:i + 8]
        lines.append("    " + ",".join(
            "0x%02x,0x%02x" % (rgb565(r, g, b) & 0xFF, rgb565(r, g, b) >> 8)
            for r, g, b in chunk
        ) + ",")
    body = "\n".join(lines)
    with open(path, "w", encoding="utf-8", newline="\n") as f:
        f.write(f"""\
/* 背景图: 由 gen_bg_img.py 从 AiDoGUI 导出的 ARGB8888 (658x987) 转换为
 * {DST_W}x{DST_H} RGB565 (小端字节序)。原图超出 flash 预算, 转换后再进固件。
 * Copyright (c) 2026-2050 AiDoGUI (原始导出素材), 转换脚本见 gen_bg_img.py。 */
#include "lvgl.h"

#ifndef LV_ATTRIBUTE_MEM_ALIGN
#define LV_ATTRIBUTE_MEM_ALIGN
#endif

#ifndef LV_ATTRIBUTE_UI_IMG_T04F3A84E8F9CA6DA16_JPG
#define LV_ATTRIBUTE_UI_IMG_T04F3A84E8F9CA6DA16_JPG
#endif

static const
LV_ATTRIBUTE_MEM_ALIGN LV_ATTRIBUTE_LARGE_CONST LV_ATTRIBUTE_UI_IMG_T04F3A84E8F9CA6DA16_JPG
uint8_t ui_img_t04f3a84e8f9ca6da16_jpg_map[] = {{
{body}
}};

const lv_image_dsc_t ui_img_t04f3a84e8f9ca6da16_jpg = {{
  .header = {{
    .magic = LV_IMAGE_HEADER_MAGIC,
    .cf = LV_COLOR_FORMAT_RGB565,
    .flags = 0,
    .w = {DST_W},
    .h = {DST_H},
    .stride = {DST_W * 2},
  }},
  .data_size = sizeof(ui_img_t04f3a84e8f9ca6da16_jpg_map),
  .data = ui_img_t04f3a84e8f9ca6da16_jpg_map,
}};
""")


def main():
    args, opts = [], {}
    it = iter(sys.argv[1:])
    for a in it:
        if a.startswith("--"):
            k, _, v = a[2:].partition("=")
            opts[k] = v if v else next(it, "")
        else:
            args.append(a)
    if len(args) < 2:
        raise SystemExit(__doc__)
    src, dst = args[0], args[1]
    mode = opts.get("mode", "fill")
    print(f"reading {src} ...")
    blob = parse_map(src)
    print(f"resample {SRC_W}x{SRC_H} -> {DST_W}x{DST_H} mode={mode} ...")
    pixels = resample(blob, mode)
    if "preview" in opts:
        write_png(opts["preview"], pixels)
        print("preview ->", opts["preview"])
    if dst != "-":
        write_c(dst, pixels)
        print("generated ->", dst)


if __name__ == "__main__":
    main()
