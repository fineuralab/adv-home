# 第三方来源与许可

[English](NOTICE.md) | **简体中文**

本说明覆盖 ADV Home v0.1（标签 v0.1.0）的源码和固件。
各组件继续适用各自许可；根目录 MIT 不覆盖第三方代码与字体。上游原始署名予以保留。

## 原创代码与实际字体

`core`、`main`、`tests`、`tools` 中原创应用和工具适用 [MIT](LICENSE)，
其中 `core/font_data.h` 属于字体数据，适用 SIL Open Font License 1.1。
`bootloader_components/main/entry.c` 适用
[Apache-2.0](LICENSES/ESP-IDF-Apache-2.0.txt)。

Home 使用 **ADV Home UI Raster**，源于 Google Fonts 发布的未修改 Roboto 可变字体。
字体原件和 ASCII 点阵子集均保留 The Roboto Project Authors 的 2011 年版权声明及
SIL Open Font License 1.1。[许可证原文](assets/fonts/OFL.txt) · [字体来源与生成](docs/FONT-LICENSES.md)

## SDK 与运行库

使用 ESP-IDF v5.4.2 和官方 Xtensa GCC 14.2.0 / esp-14.2.0_20241119 工具链。
完整 SDK、编译器不在仓库中，链接进去的运行库包含在固件内。

- ESP-IDF 主体采用 Apache-2.0，各文件声明在
  [SDK-Source-Notices.txt](LICENSES/sdk/SDK-Source-Notices.txt)。
- FreeRTOS 采用 MIT，保留 Amazon、Espressif 等版权声明及完整许可。
- TLSF 采用 BSD-3-Clause；SD/MMC 等采用 BSD / ISC；FatFs 采用 ChaN 的宽松许可。
  条件与免责声明均保留在 SDK 声明汇总中。
- Mbed TLS 从双重许可中选择 Apache-2.0，保留上游许可文件及源码声明。
- newlib、libm、libgloss 的 SDK 与工具链版权文件分别位于 `LICENSES/sdk`、
  `LICENSES/toolchain`。
- GCC libgcc / libstdc++ 采用 GPL-3.0 加 GCC Runtime Library Exception 3.1，
  两份完整文本和头文件声明位于 `LICENSES/toolchain`。符合该例外条件的独立应用可按
  自身条款分发，运行库本身的许可不变。本项目使用未修改的官方工具链构建。
- LLVM 派生头文件采用 Apache-2.0 加 LLVM 例外；其他 Xtensa 支持部分保留各自声明。

## 经本地裁剪的 M5GFX

基于官方 M5GFX 0.2.15，提交 c6f92dc03226cdc04d67c705a2020f62ad21ad01。
这是**本地修改的子集**：移除未使用的可选字库和示例，保留默认 GLCD 字库及图形代码。
四个修改文件带有修改说明。详见[裁剪说明](docs/M5GFX-SUBSET.md)、
[逐文件清单](docs/m5gfx-subset.json)及 [third-party.json](third-party.json)。

保留 M5Stack 的 MIT、LovyanGFX / TFT_eSPI 的 BSD、Adafruit GLCD 的 BSD，以及
TJpgDec、Pngle、QRCode、result.hpp、miniz 等各自条款与署名。完整文本在
`LICENSES/M5GFX`，原文件声明也继续保留。
[Source-Notices.txt](LICENSES/M5GFX/Source-Notices.txt)只汇总当前保留源码的声明。

当前分发不再包含 IPA、GNU FreeFont、Orbitron 等可选字体程序、声明、原字体资料包
或示例，因此当前源码包无需为这些已移除资源提供 IPA 替换方案或判断 Orbitron 保留名用法。
通用字体渲染类不代表附带这些字库；恢复它们时需重新审查相应许可。

## 核对与再分发

SDK 与工具链声明保守覆盖原 N1.1 链接映射和编译依赖，SDK 及配置保持固定；
裁剪后的 M5GFX 另行比对并验证构建。该清单不是逐字节软件物料表。
[许可材料清单](docs/license-inventory.json)记录来源、版本、摘要、字体命名与裁剪决策。
摘要检查不能替代更换依赖时对实际许可条件的审查。

完整包包含整个 LICENSES 目录、字体原文件和上述说明。转发独立 bin 时请附上对应的
third-party-notices ZIP。当前版本替代此前打包修订；历史标签与附件作为存档保留，
其包含的资源与当前裁剪版本不同。

M5Stack、Cardputer、Espressif、Roboto 等名称属于各自权利人。键位对应参考
[M5Stack 官方实现](https://github.com/m5stack/M5Cardputer/tree/master/src/utility/Keyboard)。
ADV Home 为独立项目，署名不表示上游认可或背书。
