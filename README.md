# vEdge 1000 OpenWrt

适用于 Cisco vEdge 1000（Cavium CN6130，OpenWrt 目标 `octeon/generic`）的定制固件。两个基线都基于 OpenWrt 25.12.6 和 Linux 6.12.112。

| 基线 | 主要功能 | 固件附件 |
| --- | --- | --- |
| [v1.0.0](https://github.com/yzs18994078313-byte/vedge1000-openwrt/releases/tag/v1.0.0) | 八个 SFP 口的 EEPROM 模式识别、SGMII/1000BASE-X 和热插拔修复、SFP 面板 LED、中文 LuCI、Argon、OpenClash/Mihomo、WireGuard、DDNS、SQM 和 USB 存储 | 升级包 `.tar`、RAM 启动镜像 `.bin` |
| [v1.1.0](https://github.com/yzs18994078313-byte/vedge1000-openwrt/releases/tag/v1.1.0) | 延续 v1.0.0，新增 Octeon SHA/AES-CBC 加速、libtorrent 硬件 SHA、qBittorrent 5.3.0-r8、外置盘 HTTP 下载、DB-IP、自动挂载、LuCI 磁盘管理及 SFTP | 自本版本以后不再提供RAM启动镜像，因为大小已超出UBOOT安全边界升级包 `.tar`已在release页面发布 |

- [v1.0.0 功能与附件说明](docs/releases/v1.0.0.md)
- [v1.1.0 功能与附件说明](docs/releases/v1.1.0.md)

每个标签中的 `source/` 对应该基线的 OpenWrt 源码、feeds、内核补丁和构建配置。完整构建依赖和缓存见各 Release 的 `*-source.tar.zst`。下载固件后请按对应 Release 的 SHA-256 清单核对。
