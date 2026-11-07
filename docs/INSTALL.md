# 安装与使用

适用硬件为 M5Stack Cardputer ADV，ESP32-S3，8 MB Flash。屏幕显示 v0.1，对应 Git 标签 v0.1.0。

## 1. 选择文件

| 当前设备状态 | 使用文件 | 地址 | Flash 应用和 Home 设置 |
|---|---|---|---|
| 首次安装 / 其他启动器 / 布局不确定 | ADV-Home-v0.1_0x0.bin | 0x0 | 重建，原有内容需备份 |
| 已经安装 ADV Home N1 / N1.1 / N1.2 | ADV-Home-v0.1-update_0x10000.bin | 0x10000 | 保留 |

更新文件仅适用于现有 ADV Home 分区布局。请按上表给文件填写对应的烧录地址。

下载后可在 Mac 用以下命令核对发布包：

~~~sh
shasum -a 256 -c SHA256SUMS.txt
~~~

## 2. 进入下载模式并烧录

1. 停止记录等正在写入的数据操作，备份所需内容。拔掉 USB，电源开关设为 OFF。
2. 按住 G0 / BOOT，接上 USB 数据线，再松开按键。
3. 用 Chrome 或 Edge 打开 [Espressif 网页烧录器](https://espressif.github.io/esptool-js/)，连接 USB JTAG/serial debug unit 对应端口。在 Mac 上通常名为 cu.usbmodem…。
4. 文件列表只添加本次需要的一个文件，按上表填写地址。建议波特率 460800；可配置时选 DIO / 80 MHz / 8 MB。
5. 更新 Home 时不要勾选 Erase all。完整包已包含新的分区与默认配置，会重建 A/B 应用位置和 Flash 配置。
6. 点击 Program，等待写入与校验完成，断开串口连接。
7. 拔掉 USB，将开关设为 ON 正常启动，不按 G0。首页版本应为 v0.1。

烧录过程不修改 TF 卡文件。

## 3. 从 TF 卡安装应用

1. TF 卡使用 FAT32；推荐 MBR 分区布局。将兼容的应用 .bin 文件复制到卡中。
2. 在电脑上安全推出卡，设备断电时插入。
3. 进入 SD card，选择文件，再选择 App A 或 App B。
4. 确认页初始选中 Cancel。按右方向切到 Install，再按 Enter。
5. 等待 App installed。进入 Apps，选择对应位置，再选 Run app。

替换已有应用会覆盖所选位置；另一位置保留。安装时保持供电，等待完成或错误提示后再操作。

**安装成功会把默认应用改为刚安装的位置。** 如果希望始终启动 App A，安装 App B 后请重新到 Settings 选择 A。

## 4. 默认启动与返回 Home

在 Settings 中设置 Default app 为 A 或 B，并开启 Auto start。所选位置已有可运行应用时，下次正常启动显示 3 秒倒计时，随后进入该应用。

需要留在 Home 时，在倒计时期间按任意键。应用运行中按 RST 复位，再在倒计时中按键即可。异常复位会跳过自动启动。

## 5. 导航和故障提示

- 左 / 右：逗号 / 斜杠；上 / 下：分号 / 句号。无需 Fn。
- Enter：打开或确认；反引号键 / Backspace：返回。
- SD 子目录返回上一级，SD 根目录返回 Home；应用操作页直接返回 Home。
- Incomplete 表示上次安装未完成，可从 TF 卡重新安装该位置。
- Needs extra data partitions 表示该整合包依赖专用数据分区，当前安装器不支持。
- 安装失败会停在提示页，记录提示文字后再按 Enter 或返回。

需要进一步定位时，正常启动设备后连接串口终端，不按 G0，并先关闭烧录网页的串口连接。串口提供安装阶段、SDK 错误名、任务栈余量与复位原因。串口导航也支持 a/d、w/s、Enter、q。

## 6. 更换启动器

如需恢复其他启动器，请从其官方或可信发布页获取适配 Cardputer ADV 的完整烧录镜像，按对应项目的地址说明重新烧录。应用与设置可能需要重新安装；先备份所需内容。
