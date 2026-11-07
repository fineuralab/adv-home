# 开发与发布

## 固定依赖

- ESP-IDF v5.4.2；[官方安装文档](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32s3/get-started/index.html)。
- 经本地裁剪的 M5GFX 0.2.15，源码位于 components/M5GFX；来源、原始及当前目录摘要见 third-party.json，改动见 docs/m5gfx-subset.json。
- Python 3.9+、Clang / Clang++：用于主机测试。
- Pillow：仅用于重新生成字体或预览，固件构建不需要。

本仓库不依赖上级目录的工具脚本、其他固件或个人虚拟环境。使用已激活的 ESP-IDF 环境执行 README 中的构建命令即可。

## 代码结构

| 目录或文件 | 用途 |
|---|---|
| core/image.* | 流式 SHA-256、镜像解析、安装事务 |
| core/model.* | 导航、确认、设置、自动启动状态机 |
| core/view.* | RGB565 帧缓冲与页面绘制 |
| core/boot_request.h | 一次性 RTC 启动请求 |
| main/hardware.* | 显示、键盘和 SD 卡适配 |
| main/storage.* | NVS 记录、应用擦写与读回校验 |
| bootloader_components/main/entry.c | 自定义二阶段启动选择 |
| tests / tools | 主机测试、栈帧检查、镜像验证与发布打包 |

## 分区

| 地址 | 大小 | 内容 |
|---|---|---|
| 0x0 | 小于 32 KiB | 二阶段引导 |
| 0x8000 | 4 KiB | 分区表 |
| 0x9000 | 20 KiB | 普通应用 NVS |
| 0xE000 | 8 KiB | SDK OTA 状态区 |
| 0x10000 | 960 KiB | Home |
| 0x100000 | 3 MiB | App A |
| 0x400000 | 3 MiB | App B |
| 0x700000 | 24 KiB | Home 配置与应用记录 |

两个应用位置均为固定大小；带专用文件系统的整合包会被拒绝。Home 配置区与应用默认 NVS 分开存储。

## 软件检查

~~~sh
python3 tools/test.py
python3 tools/test_bootloader.py
python3 tools/check_stack.py --build build --check
~~~

前两项使用实际生产代码与 SDK API 替身运行 ASan / UBSan。第三项需要 ESP-IDF 的 Xtensa objdump，读取实际 ELF 的函数序言，检查安装嵌套调用链的栈帧下限，并留出 2048 字节余量。它不能替代实机栈峰值测量。

主机测试默认生成独立样本。需要额外检查某个本地固件时，可显式传入：

~~~sh
python3 tools/test.py --firmware path/to/application.bin
~~~

额外文件的检查结果仅写入本地测试输出，不会自动进入 Release。

## 创建本地发布文件

构建后生成完整镜像，并打包：

~~~sh
idf.py merge-bin -o ADV-Home-v0.1_0x0.bin -f raw
python3 tools/make_release.py --build build --out dist
~~~

此处 merge-bin 的相对输出位置为 build/，因此 -o 只写文件名；后续校验与打包从 build/ 读取该文件。

工具先检查 ESP32-S3 镜像的分段、校验和、SHA-256、分区表和完整包中的默认配置，再用明确的文件清单创建：

- 完整包与更新包；
- 独立源码 ZIP；
- 含固件、安装说明、许可证、预览和内部校验表的下载 ZIP；
- 适用于整个下载目录的 SHA256SUMS.txt。

源码导出仅包含顶层文档、配置，以及 core、main、bootloader_components、components、assets、tests、tools、docs、LICENSES 和 .github。构建目录、日志、虚拟环境、Git 数据库、macOS 元数据与个人环境文件不导出。目录中的 .gitignore 和 .github 为正常项目配置。

打包只读取本地构建产物，不烧录设备、不连接 GitHub，也不上传文件。输出目录必须不存在，避免覆盖已有发布。

生成的 manifest.json 只记录版本、硬件、烧录地址和文件摘要，不记录主机目录或 Docker 信息。二进制仍可能包含通用 SDK / 工具链断言路径，以及编译时间和 SDK 版本。

## 对应版本

v0.1 由对应源码构建。重新编译会因时间、路径或环境等因素得到不同摘要，应以当次打包生成的校验表为准。

GitHub Actions 配置会运行主机测试、ESP-IDF 构建、栈帧检查和镜像检查。远端运行结果以仓库的 Actions 页面为准。

## 第三方许可材料

v0.1 的打包标签为 `v0.1.0`。

~~~sh
python3 tools/check_notices.py
python3 tools/make_release.py --build build --out dist-notices
~~~

`check_notices.py` 核对许可材料清单中的文件摘要、LICENSES 和 assets/fonts 目录，以及
固定 M5GFX 源码树摘要。打包必须通过该检查，归档中包括许可子目录和字体来源，
并额外生成可随独立 bin 一起提供的 third-party-notices ZIP。CI 同时检查此打包流程。

`docs/license-inventory.json` 根据原 N1.1 链接映射、对应 SDK 源码候选、编译依赖与
完整 M5GFX 源码整理；其范围说明见 NOTICE.md。SDK 与工具链的原始声明是按原文件注释
收集的，M5GFX 使用本地裁剪子集。Home 界面使用 Roboto 的 ASCII 点阵子集，点阵、字体原件和许可证均纳入摘要检查。字体来源和生成方式见 [字体说明](FONT-LICENSES.md)。

调整依赖、编译配置或字体时，应重新核对其许可与来源，再更新清单；不能仅更新摘要来
跳过核对。检查工具校验已审查材料的一致性，不能自动判定任意新增代码的许可。
发布修订使用新标签与新目录，保留已公开标签、附件和校验值，避免覆盖历史下载。
