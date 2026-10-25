# -*- coding: utf-8 -*-
"""
boot_logo_gen.py — 开机 LOGO 位图生成器 (跑一次即可, 产出物已入库)

从 PDF 内嵌图片提取 LOGO -> 裁切内容外框 -> 区域平均缩放 -> 黑场归一 ->
输出 LVGL 9 RGB565 图片描述符 (main/boot_logo.c/h)。

用法 (仓库根目录):
    python -I main/boot_logo_gen.py

源图: E:/esp/视频/图片转PDF.pdf (ZY 电路板风格 LOGO, 784x634 RGB)
"""
import os
import re
import struct
import sys
import zlib

PDF_PATH = r"E:\esp\视频\图片转PDF.pdf"
PREVIEW_PATH = r"E:\esp\视频\boot_logo_preview.png"
SRC_W, SRC_H = 784, 634          # PDF 图片 XObject 尺寸
MAX_W, MAX_H = 240, 160          # 目标框 (320x240 横屏内居中)
BLACK_CUTOFF = 16                # 亮度低于此值归为纯黑, 避免图片底色与屏底色接缝


def extract_rgb_from_pdf(path):
    data = open(path, "rb").read()
    i = data.find(b"7 0 obj")            # 图片 XObject
    s = data.find(b"stream", i)
    p = s + 6
    if data[p:p + 2] == b"\r\n":
        p += 2
    elif data[p:p + 1] == b"\n":
        p += 1
    d = zlib.decompressobj()
    raw = d.decompress(data[p:p + 168256])   # /Length
    raw += d.flush()
    assert len(raw) >= SRC_W * SRC_H * 3, "PDF 图片流尺寸异常"
    return raw[: SRC_W * SRC_H * 3]


def content_bbox(raw, w, h, thresh=24):
    minx, miny, maxx, maxy = w, h, -1, -1
    for y in range(h):
        row = raw[y * w * 3:(y + 1) * w * 3]
        for x in range(w):
            if row[x * 3] + row[x * 3 + 1] + row[x * 3 + 2] > thresh * 3:
                if x < minx:
                    minx = x
                if x > maxx:
                    maxx = x
                if y < miny:
                    miny = y
                if y > maxy:
                    maxy = y
    return minx, miny, maxx, maxy


def area_resize(raw, sw, sh, dw, dh):
    """区域平均缩放 (box filter), 大比例缩小比双线性更干净"""
    out = bytearray(dw * dh * 3)
    for dy in range(dh):
        sy0 = dy * sh / dh
        sy1 = (dy + 1) * sh / dh
        y0, y1 = int(sy0), max(int(sy0) + 1, int(sy1 + 0.999))
        for dx in range(dw):
            sx0 = dx * sw / dw
            sx1 = (dx + 1) * sw / dw
            x0, x1 = int(sx0), max(int(sx0) + 1, int(sx1 + 0.999))
            rs = gs = bs = n = 0
            for y in range(y0, min(y1, sh)):
                base = y * sw * 3
                for x in range(x0, min(x1, sw)):
                    o = base + x * 3
                    rs += raw[o]
                    gs += raw[o + 1]
                    bs += raw[o + 2]
                    n += 1
            o = (dy * dw + dx) * 3
            out[o] = rs // n
            out[o + 1] = gs // n
            out[o + 2] = bs // n
    return out


def to_rgb565(raw):
    words = []
    for o in range(0, len(raw), 3):
        r, g, b = raw[o], raw[o + 1], raw[o + 2]
        if r < BLACK_CUTOFF and g < BLACK_CUTOFF and b < BLACK_CUTOFF:
            words.append(0)
            continue
        words.append(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))
    return words


def write_png(path, raw, w, h):
    def chunk(tag, payload):
        return (struct.pack(">I", len(payload)) + tag + payload
                + struct.pack(">I", zlib.crc32(tag + payload) & 0xFFFFFFFF))

    scan = b"".join(b"\x00" + bytes(raw[y * w * 3:(y + 1) * w * 3]) for y in range(h))
    ihdr = struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0)
    png = (b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr)
           + chunk(b"IDAT", zlib.compress(scan, 6)) + chunk(b"IEND", b""))
    open(path, "wb").write(png)


def write_c_files(words, w, h, outdir):
    lines = []
    for i in range(0, len(words), 16):
        lines.append("    " + ", ".join("0x%04X" % v for v in words[i:i + 16]) + ",")
    body = "\n".join(lines)

    open(outdir + "/boot_logo.c", "w", encoding="utf-8", newline="\n").write(
        """/**
 * @file boot_logo.c
 * @brief 开机 LOGO 位图 (RGB565) — 由 main/boot_logo_gen.py 生成, 请勿手改
 *
 * 源: E:/esp/视频/图片转PDF.pdf 内嵌 ZY 电路板风格 LOGO
 * 生成: python -I main/boot_logo_gen.py
 */
#include "boot_logo.h"

static const uint16_t boot_logo_map[%d] = {
%s
};

const lv_image_dsc_t boot_logo = {
    .header = {
        .magic = LV_IMAGE_HEADER_MAGIC,
        .cf = LV_COLOR_FORMAT_RGB565,
        .flags = 0,
        .w = %d,
        .h = %d,
        .stride = %d,
    },
    .data_size = sizeof(boot_logo_map),
    .data = (const uint8_t *)boot_logo_map,
};
""" % (len(words), body, w, h, w * 2))

    open(outdir + "/boot_logo.h", "w", encoding="utf-8", newline="\n").write(
        """/**
 * @file boot_logo.h
 * @brief 开机 LOGO 位图 (%dx%d RGB565) — 由 main/boot_logo_gen.py 生成
 */
#ifndef BOOT_LOGO_H
#define BOOT_LOGO_H

#include "lvgl.h"

extern const lv_image_dsc_t boot_logo;

#endif /* BOOT_LOGO_H */
""" % (w, h))


def main():
    outdir = os.path.dirname(os.path.abspath(__file__))
    raw = extract_rgb_from_pdf(PDF_PATH)
    minx, miny, maxx, maxy = content_bbox(raw, SRC_W, SRC_H)
    print("content bbox: (%d,%d)-(%d,%d)  %dx%d"
          % (minx, miny, maxx, maxy, maxx - minx + 1, maxy - miny + 1))

    # 外扩 2% 余量再裁切
    pad_x = max(2, (maxx - minx) // 50)
    pad_y = max(2, (maxy - miny) // 50)
    x0 = max(0, minx - pad_x)
    y0 = max(0, miny - pad_y)
    x1 = min(SRC_W - 1, maxx + pad_x)
    y1 = min(SRC_H - 1, maxy + pad_y)
    cw, ch = x1 - x0 + 1, y1 - y0 + 1

    # 按比例缩放进 MAX_W x MAX_H 框
    scale = min(MAX_W / cw, MAX_H / ch)
    dw = max(1, int(cw * scale + 0.5))
    dh = max(1, int(ch * scale + 0.5))
    print("crop %dx%d -> out %dx%d" % (cw, ch, dw, dh))

    cropped = bytearray()
    for y in range(y0, y1 + 1):
        cropped += raw[(y * SRC_W + x0) * 3:(y * SRC_W + x1 + 1) * 3]
    resized = area_resize(bytes(cropped), cw, ch, dw, dh)
    words = to_rgb565(resized)

    write_c_files(words, dw, dh, outdir)
    write_png(PREVIEW_PATH, resized, dw, dh)
    print("wrote %s/boot_logo.c (+ .h), preview -> %s" % (outdir, PREVIEW_PATH))


if __name__ == "__main__":
    main()
