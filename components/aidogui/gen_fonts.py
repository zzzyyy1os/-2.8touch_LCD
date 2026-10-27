#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""生成天气界面动态文本宽字库 weather_font_16/30/60.c (lv_font_conv)。

AiDoGUI 导出的 ui_font___* 只含设计稿出现过的字, 城市名/天气词/任意数字
会缺字显示空白, 所以另生成宽字库给动态控件用 (weather_ui.c 运行时替换)。

字符集 = ASCII + ° + 标签(体感湿度温度) + 日期(周X月日) + 天气种类表用字
        + 风向风力 + 全国城市名/省份名用字。
weather_font_60 只带时钟/大温度用的数字和符号, font16/30 带全部。

用法: python -I gen_fonts.py     (需要 Node/npx, 首次运行会下载 lv_font_conv)
输出: weather_font_16.c / weather_font_30.c / weather_font_60.c (同目录)
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
TOOLS_DIR = os.path.join(HERE, ".lvfont_tools")
LV_FONT_CONV_JS = os.path.join(TOOLS_DIR, "node_modules", "lv_font_conv", "lv_font_conv.js")

# 中文字体源, 依次尝试 (优先用户下载的南西油墨宋)
FONT_CANDIDATES = [
    "E:/新建文件夹/南西油墨宋.ttf",
    "C:/Windows/Fonts/msyh.ttc",
    "C:/Windows/Fonts/simhei.ttf",
]

# 补丁字库源: 南西油墨宋部分细笔画标点 (" % * - < > |) freetype 渲染出空
# 位图被 lv_font_conv 跳过 (字形整个不存在, 运行时显示方框), 缺字从黑体
# 生成补丁字库, 主字库 .fallback 指向补丁 (LVGL 缺字自动回退)
PATCH_FONT = "C:/Windows/Fonts/simhei.ttf"

# 天气种类表用字 (聚合数据 wid 列表 + "转")
WTYPE = "晴多云阴阵雨雷伴有冰雹雪夹小中大暴特到雾冻沙尘浮扬强霾转"
# 风向/风力
WIND = "东南西北风级微"
# 界面标签 (含查询期占位 "加载中...")
LABEL = "体感湿度温度加载中"
# 日期
DATE = "周一二三四五六日月"

# 全国地级市 + 直辖市/港澳台主要城市 + 省份名 (IP 定位返回啥都能显示;
# 城市名去"市"后缀, 与聚合数据 V3 返回格式一致)
CITIES = """
北京 天津 上海 重庆
石家庄 唐山 秦皇岛 邯郸 邢台 保定 张家口 承德 沧州 廊坊 衡水
太原 大同 阳泉 长治 晋城 朔州 晋中 运城 忻州 临汾 吕梁
呼和浩特 包头 乌海 赤峰 通辽 鄂尔多斯 呼伦贝尔 巴彦淖尔 乌兰察布 兴安 锡林郭勒 阿拉善
沈阳 大连 鞍山 抚顺 本溪 丹东 锦州 营口 阜新 辽阳 盘锦 铁岭 朝阳 葫芦岛
长春 吉林 四平 辽源 通化 白山 松原 白城 延边
哈尔滨 齐齐哈尔 鸡西 鹤岗 双鸭山 大庆 伊春 佳木斯 七台河 牡丹江 黑河 绥化 大兴安岭
南京 无锡 徐州 常州 苏州 南通 连云港 淮安 盐城 扬州 镇江 泰州 宿迁
杭州 宁波 温州 嘉兴 湖州 绍兴 金华 衢州 舟山 台州 丽水
合肥 芜湖 蚌埠 淮南 马鞍山 淮北 铜陵 安庆 黄山 滁州 阜阳 宿州 六安 亳州 池州 宣城
福州 厦门 莆田 三明 泉州 漳州 南平 龙岩 宁德
南昌 景德镇 萍乡 九江 新余 鹰潭 赣州 吉安 宜春 抚州 上饶
济南 青岛 淄博 枣庄 东营 烟台 潍坊 济宁 泰安 威海 日照 临沂 德州 聊城 滨州 菏泽
郑州 开封 洛阳 平顶山 安阳 鹤壁 新乡 焦作 濮阳 许昌 漯河 三门峡 南阳 商丘 信阳 周口 驻马店 济源
武汉 黄石 十堰 宜昌 襄阳 鄂州 荆门 孝感 荆州 黄冈 咸宁 随州 恩施 仙桃 潜江 天门 神农架
长沙 株洲 湘潭 衡阳 邵阳 岳阳 常德 张家界 益阳 郴州 永州 怀化 娄底 湘西
广州 韶关 深圳 珠海 汕头 佛山 江门 湛江 茂名 肇庆 惠州 梅州 汕尾 河源 阳江 清远 东莞 中山 潮州 揭阳 云浮
南宁 柳州 桂林 梧州 北海 防城港 钦州 贵港 玉林 百色 贺州 河池 来宾 崇左
海口 三亚 三沙 儋州
成都 自贡 攀枝花 泸州 德阳 绵阳 广元 遂宁 内江 乐山 南充 眉山 宜宾 广安 达州 雅安 巴中 资阳 阿坝 甘孜 凉山
贵阳 六盘水 遵义 安顺 毕节 铜仁 黔西南 黔东南 黔南
昆明 曲靖 玉溪 保山 昭通 丽江 普洱 临沧 楚雄 红河 文山 西双版纳 大理 德宏 怒江 迪庆
拉萨 日喀则 昌都 林芝 山南 那曲 阿里
西安 铜川 宝鸡 咸阳 渭南 延安 汉中 榆林 安康 商洛
兰州 嘉峪关 金昌 白银 天水 武威 张掖 平凉 酒泉 庆阳 定西 陇南 临夏 甘南
西宁 海东 海北 黄南 海南 果洛 玉树 海西
银川 石嘴山 吴忠 固原 中卫
乌鲁木齐 克拉玛依 吐鲁番 哈密 昌吉 博尔塔拉 巴音郭楞 阿克苏 克孜勒苏 喀什 和田 伊犁 塔城 阿勒泰
香港 澳门 台北 高雄 台中 台南 新竹 嘉义
昆山 义乌 江阴 常熟 张家港 慈溪 诸暨 瑞安 乐清 温岭
河北 山西 辽宁 吉林 黑龙江 江苏 浙江 安徽 福建 江西 山东 河南 湖北 湖南 广东 海南 四川 贵州 云南 陕西 甘肃 青海 台湾 内蒙古 广西 西藏 宁夏 新疆
"""


def build_charset(include_cities):
    chars = set(chr(c) for c in range(0x20, 0x7F))   # ASCII 可见字符
    chars |= set("°") | set(WTYPE) | set(WIND) | set(LABEL) | set(DATE)
    if include_cities:
        chars |= set(CITIES) - set(" \n")
    return "".join(sorted(chars))


def pick_font():
    for p in FONT_CANDIDATES:
        if os.path.isfile(p):
            return p
    raise SystemExit("no CJK ttf found, install one of: " + ", ".join(FONT_CANDIDATES))


def ensure_tool():
    """本地安装 lv_font_conv (只装一次)。用 node 直接调 js 入口:
    走 npx.cmd 批处理时, --symbols 里的 > | & 等会被 cmd.exe 当特殊字符,
    中文参数还可能被 GBK 转码搞坏, 直接起 node.exe 则参数原样送达。"""
    if os.path.isfile(LV_FONT_CONV_JS):
        return
    npm = shutil.which("npm") or shutil.which("npm.cmd") or "npm"
    print("installing lv_font_conv ...")
    r = subprocess.run([npm, "install", "lv_font_conv", "--prefix", TOOLS_DIR],
                       capture_output=True)
    if r.returncode != 0 or not os.path.isfile(LV_FONT_CONV_JS):
        log = os.path.join(TOOLS_DIR, "npm_install_err.txt")
        os.makedirs(TOOLS_DIR, exist_ok=True)
        with open(log, "wb") as f:
            f.write(r.stdout + b"\n---STDERR---\n" + r.stderr)
        raise SystemExit(f"npm install lv_font_conv failed, see {log}")


def run_tool(args, out_path):
    node = shutil.which("node") or "node"
    r = subprocess.run([node, LV_FONT_CONV_JS] + args, capture_output=True)
    if r.returncode != 0 or not os.path.isfile(out_path):
        log = os.path.splitext(out_path)[0] + "_err.txt"
        with open(log, "wb") as f:
            f.write(r.stdout + b"\n---STDERR---\n" + r.stderr)
        raise SystemExit(f"lv_font_conv failed, see {log}")


def gen_one(font_file, size, chars, out_path, symbol, full_ascii=True):
    args = [
        "--font", font_file,
        "--size", str(size),
        "--bpp", "4",
        "--format", "lvgl",
        "--no-compress",
    ]
    if full_ascii:
        args += ["-r", "0x20-0x7E"]        # font16/30 会有任意数字/标点, 备全 ASCII
    args += [
        "--symbols", chars,
        "--lv-font-name", symbol,
        "-o", out_path,
    ]
    print(f"[{symbol}] {len(chars)} glyphs, size={size} -> {out_path}")
    run_tool(args, out_path)

    # 符号名规整成约定的 weather_font_NN, 并保证 const
    with open(out_path, "r", encoding="utf-8") as f:
        src = f.read()
    src = re.sub(
        r"(?m)^(const\s+)?lv_font_t\s+\w+\s*=",
        f"const lv_font_t {symbol} =",
        src,
        count=1,
    )
    if f"const lv_font_t {symbol}" not in src:
        raise SystemExit(f"symbol patch failed for {symbol}, check {out_path}")
    with open(out_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(src)
    print(f"[{symbol}] ok ({os.path.getsize(out_path) // 1024} KB)")


def detect_missing(out_path, want):
    """对比 lv_font_conv 输出的 U+XXXX 字形注释, 返回未生成的字符"""
    with open(out_path, "r", encoding="utf-8") as f:
        src = f.read()
    have = set(re.findall(r"U\+([0-9A-F]{4})", src))
    return "".join(sorted(c for c in set(want) if "%04X" % ord(c) not in have))


def link_fallback(font_c_path, patch_symbol):
    """主字库 .fallback 指向补丁字库 (LVGL 缺字自动回退)"""
    with open(font_c_path, "r", encoding="utf-8") as f:
        src = f.read()
    needle = "    .fallback = NULL,"
    if needle not in src:
        raise SystemExit(f"fallback slot not found in {font_c_path}")
    decl = f"LV_FONT_DECLARE({patch_symbol});\n"
    anchor = "/*Initialize a public general font descriptor*/"
    src = src.replace(anchor, decl + "\n" + anchor, 1)
    src = src.replace(needle, f"    .fallback = &{patch_symbol},", 1)
    with open(font_c_path, "w", encoding="utf-8", newline="\n") as f:
        f.write(src)


def main():
    ensure_tool()
    font_file = pick_font()
    print("font:", font_file)
    full = build_charset(include_cities=True)
    light = build_charset(include_cities=False)
    ascii7 = "".join(chr(c) for c in range(0x20, 0x7F))
    jobs = [
        (30, full, True),
        (16, light, True),
        # font60 只用在时钟/大温度 ("14" ":36" "26°" "-3°"), 不备全 ASCII 省 flash
        (60, "0123456789:.°-", False),
    ]

    missing = set()
    for size, chars, full_ascii in jobs:
        symbol = f"weather_font_{size}"
        out = os.path.join(HERE, symbol + ".c")
        gen_one(font_file, size, chars, out, symbol, full_ascii=full_ascii)
        want = chars + (ascii7 if full_ascii else "")
        miss = detect_missing(out, want)
        if miss:
            print(f"[{symbol}] font source missing glyphs: {miss!r} -> patch")
            missing |= set(miss)

    if not missing:
        print("no missing glyphs, patch fonts not needed")
        return
    patch_chars = "".join(sorted(missing))
    for size, _, _ in jobs:
        symbol = f"weather_font_{size}"
        patch_symbol = f"weather_font_patch_{size}"
        gen_one(PATCH_FONT, size, patch_chars,
                os.path.join(HERE, patch_symbol + ".c"), patch_symbol, full_ascii=False)
        link_fallback(os.path.join(HERE, symbol + ".c"), patch_symbol)
    print("fallback patch done:", patch_chars)


if __name__ == "__main__":
    main()
