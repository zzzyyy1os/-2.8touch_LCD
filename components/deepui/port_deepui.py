#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""把 AiDoGUI 导出的 DEEPSEEK 余额界面移植成 ESP-IDF 组件 components/deepui。

AiDoGUI 两个工程的导出符号完全同名 (aidogui_init / aidogui_ui / ui_screen1_* /
ui_font___* ...), 链接必撞, 所以移植时把 DEEP 这份的符号整体加 deepui_ 前缀;
文件名同步改, 保证 "ui.h" 这类头文件名也唯一 (main 可同时包含两套 UI 头)。

背景图与天气界面共用同一素材 (t04f3a84e8f9ca6da16): 像素数组留在
components/aidogui (gen_bg_img.py 产物, 339x254 RGB565), DEEP 屏幕从
(-9,-14) 铺开正好盖满 320x240, 不再存第二份 (flash 放不下)。

用法: python -I port_deepui.py [导出目录]   (默认 E:/LVGL_out/DEEP)
重导出后重跑本脚本即可 (末尾自动跑 gen_deep_fonts.py 补字库链接)。
"""
import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
DEFAULT_SRC = "E:/LVGL_out/DEEP"

# 内容替换 (有序, 全部带词边界回看, 保证重跑不再叠一层前缀)。
# ui_img_* 像素符号刻意不改名: 与 components/aidogui 共享同一张背景图;
# get_glyph_bitmap/dsc 是 LVGL 字库描述符字段名, 不能动。
RULES = [
    ("_TOKEN_UI_UI_H", "_TOKEN_DEEPUI_UI_H"),     # ui.h 头保护宏, 与天气界面同名
    # DEEP 导出按 32bit 色深生成, 本工程屏是 RGB565 (与天气界面一致)
    ("LV_COLOR_DEPTH != 32", "LV_COLOR_DEPTH != 16"),
    ("LV_COLOR_DEPTH should be 32bit", "LV_COLOR_DEPTH should be 16bit"),
    ("LV_EVENT_GET_COMP_CHILD", "DEEPUI_EVENT_GET_COMP_CHILD"),
    # 生成器公共枚举/类型/回调, 不带组件前缀, 两套导出全同名 (编译/链接必撞)
    ("_UI_", "_DEEPUI_"),                         # _UI_MODIFY_* / _UI_*_PROPERTY_*
    ("UI_", "DEEPUI_UI_"),                        # UI_MOVE_CURSOR_* / UI_SCREEN1_H / UI_FONT___* 头保护
    ("ui_modify_flag", "deepui_modify_flag"),
    ("ui_modify_state", "deepui_modify_state"),
    ("ui_move_cursor", "deepui_move_cursor"),
    ("ui_anim_user_data", "deepui_anim_user_data"),
    ("get_component_child_event_cb", "deepui_get_component_child_event_cb"),
    ("del_component_child_event_cb", "deepui_del_component_child_event_cb"),
    ("aido_event_", "deepui_event_"),
    ("aidogui_", "deepui_"),
    ("AIDOGUI_", "DEEPUI_"),
    ("ui_comp_hook", "deepui_comp_hook"),
    ("ui_comp", "deepui_comp"),
    ("ui_helpers", "deepui_helpers"),
    ("ui_comm", "deepui_comm"),
    ("ui_animations", "deepui_animations"),
    ("ui_events", "deepui_events"),
    ("ui_fonts", "deepui_fonts"),
    ("ui_font___", "deepui_font___"),
    ("ui_images", "deepui_images"),
    ("ui_screen1", "deepui_screen1"),
    ("custom_init", "deepui_custom_init"),
    ("_ui_", "_deepui_"),
    ('"ui.h"', '"deepui.h"'),
    ('"../ui.h"', '"../deepui.h"'),
]

# 文件名映射 (其余文件名不变)
FILE_RENAMES = {
    "ui.c": "deepui.c", "ui.h": "deepui.h",
    "ui_helpers.c": "deepui_helpers.c", "ui_helpers.h": "deepui_helpers.h",
    "ui_comm.c": "deepui_comm.c", "ui_comm.h": "deepui_comm.h",
    "aidogui_objects.h": "deepui_objects.h",
    "ui_comp.c": "deepui_comp.c", "ui_comp.h": "deepui_comp.h",
    "ui_comp_hook.c": "deepui_comp_hook.c", "ui_comp_hook.h": "deepui_comp_hook.h",
    "ui_screen1.c": "deepui_screen1.c", "ui_screen1.h": "deepui_screen1.h",
    "aido_event_screen1.c": "deepui_event_screen1.c",
    "ui_events.h": "deepui_events.h",
    "ui_fonts.c": "deepui_fonts.c", "ui_fonts.h": "deepui_fonts.h",
    "ui_font___15.c": "deepui_font___15.c",
    "ui_font___18.c": "deepui_font___18.c",
    "ui_font___20.c": "deepui_font___20.c",
    "ui_font___30.c": "deepui_font___30.c",
    "ui_font___60.c": "deepui_font___60.c",
    "ui_images.h": "deepui_images.h",
    "ui_animations.c": "deepui_animations.c", "ui_animations.h": "deepui_animations.h",
}

CMAKE_TEMPLATE = """# AiDoGUI 导出的 DEEPSEEK 余额界面 (LVGL 9, 320x240, 16bit 色深)
# port_deepui.py 从导出目录生成: 符号整体加 deepui_ 前缀与天气界面共存;
# 背景图像素共享 components/aidogui (同一素材, flash 只存一份);
# deepui_font_patch_* 是 simhei 补丁字库 (细笔画/生僻字缺失时 fallback),
# deepui_font_patch_*_x 是 arial 顶补丁 (simhei 没有的字如半角 ¥ 再回退)。
idf_component_register(
    SRCS
        "deepui.c"
        "deepui_helpers.c"
        "deepui_comm.c"
        "components/deepui_comp.c"
        "components/deepui_comp_hook.c"
        "screens/deepui_screen1.c"
        "events/deepui_event_screen1.c"
        "animations/deepui_animations.c"
        "fonts/deepui_fonts.c"
        "fonts/deepui_font___15.c"
        "fonts/deepui_font___18.c"
        "fonts/deepui_font___20.c"
        "fonts/deepui_font___30.c"
        "fonts/deepui_font___60.c"
        "fonts/deepui_font_patch_15.c"
        "fonts/deepui_font_patch_18.c"
        "fonts/deepui_font_patch_20.c"
        "fonts/deepui_font_patch_30.c"
        "fonts/deepui_font_patch_60.c"
        "fonts/deepui_font_patch_15_x.c"
        "fonts/deepui_font_patch_18_x.c"
        "fonts/deepui_font_patch_20_x.c"
        "fonts/deepui_font_patch_30_x.c"
        "fonts/deepui_font_patch_60_x.c"
        "custom/custom.c"
    INCLUDE_DIRS "." "custom"
    REQUIRES lvgl
)

# 生成代码用 "lvgl.h" 简单包含 (与 wifi_provision 组件保持一致)
target_compile_definitions(${COMPONENT_LIB} PUBLIC LV_LVGL_H_INCLUDE_SIMPLE)

# 生成代码用的 lv_obj_remove_flag 等在 LVGL 9.6 标了 deprecated, 仅告警不报错
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-deprecated-declarations)
"""

KEEP = {"port_deepui.py", "gen_deep_fonts.py"}   # 清目录时保留


def rename_content(text):
    for old, new in RULES:
        if old.startswith('"'):
            text = text.replace(old, new)         # 引号包裹的已是完整 token
        else:
            text = re.sub(r"(?<![A-Za-z0-9_])" + re.escape(old), new, text)
    return text


def main():
    src = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_SRC
    if not os.path.isdir(src):
        raise SystemExit(f"export dir not found: {src}")

    # 1) 清掉上次移植产物 (保留脚本自身)
    for name in os.listdir(HERE):
        if name in KEEP or name == "__pycache__":
            continue
        p = os.path.join(HERE, name)
        shutil.rmtree(p) if os.path.isdir(p) else os.remove(p)

    # 2) 复制导出 (背景图 13MB 原始像素不搬, 共享 aidogui 的转换版)
    for root, _dirs, files in os.walk(src):
        rel = os.path.relpath(root, src)
        dst_dir = HERE if rel == "." else os.path.join(HERE, rel.replace("\\", "/"))
        os.makedirs(dst_dir, exist_ok=True)
        for f in files:
            if f.startswith("ui_img_"):          # 像素数组共享, 不复制
                continue
            if f in ("CMakeLists.txt", "filelist.txt", ".aidogui-export.json"):
                continue
            shutil.copy2(os.path.join(root, f), os.path.join(dst_dir, f))

    # 3) 内容改名 + 文件改名
    for root, _dirs, files in os.walk(HERE):
        if os.path.basename(root) == "__pycache__":
            continue
        for f in files:
            if not f.endswith((".c", ".h")):
                continue
            p = os.path.join(root, f)
            with open(p, "r", encoding="utf-8", newline="") as fh:
                text = rename_content(fh.read())
            with open(p, "w", encoding="utf-8", newline="") as fh:
                fh.write(text)
            new_name = FILE_RENAMES.get(f, f)
            if new_name != f:
                os.replace(p, os.path.join(root, new_name))

    # 4) CMakeLists + 字库补丁
    with open(os.path.join(HERE, "CMakeLists.txt"), "w", encoding="utf-8", newline="\n") as fh:
        fh.write(CMAKE_TEMPLATE)
    subprocess.run([sys.executable, "-I", os.path.join(HERE, "gen_deep_fonts.py")], check=True)
    print("port done:", HERE)


if __name__ == "__main__":
    main()
