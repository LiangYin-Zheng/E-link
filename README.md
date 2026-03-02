# E-link

基于 ESP32-S3 的低功耗电子墨水屏显示系统

## 项目简介
E-link 是一个基于 ESP32-S3 的低功耗电子墨水屏（E-Ink）显示系统，适用于物联网、智能家居、信息展示等场景。项目支持多种尺寸的 E-Ink 屏幕，具备无线联网、远程内容推送、低功耗待机等特性。

## 特性
- 支持GDEH042Z96型号的 E-Ink 屏幕
- 基于 ESP32-S3，支持 Wi-Fi 连接
- 支持 MQTT 消息推送
- 低功耗设计，适合电池供电
- 可扩展的显示内容（图片、文本、天气等）

## 硬件需求
- ESP32-S3 开发板
- 兼容的 E-Ink 电子墨水屏（如 4.2 英寸 GDEH042Z96）
- 杜邦线若干
- USB 数据线

## 软件依赖
- Arduino IDE
- ESP32 Arduino Core
- 相关 E-Ink 屏幕驱动库

## 快速开始
1. 克隆本仓库：
	```bash
	git clone https://github.com/zc00838/ESP32-Eink-Mosaic.git
	```
2. 用 Arduino IDE 打开 `e-link.ino` 工程。
3. 配置 `credentials.h`，填写你的 Wi-Fi 和 MQTT 信息。
4. 选择对应的开发板型号（ESP32-S3），编译并上传。
5. 连接 E-Ink 屏幕，通电后即可使用。

## 目录结构
```
E-link/
├── e-link/
│   ├── e-link.ino           # 主程序入口
│   ├── credentials.h        # Wi-Fi/MQTT 配置
│   ├── epd_gdeh042Z96.h     # E-Ink 屏幕驱动
│   ├── mqtt.h               # MQTT 相关代码
│   ├── wifiConfig.h         # Wi-Fi 配置代码
│   └── ...                  # 其他头文件
└── README.md                # 项目说明文档
```

## 贡献
欢迎提交 issue 和 PR！如有建议或 bug，请在 Issues 区留言。

## 许可证
本项目采用 MIT License，详见 LICENSE 文件。

## 联系方式
如需交流或合作，请通过 GitHub Issues 联系。
