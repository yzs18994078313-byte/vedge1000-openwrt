#
# Copyright (C) 2026 OpenWrt.org
#
# This is free software, licensed under the GNU General Public License v3.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-diskman
PKG_VERSION:=1.0.0
PKG_RELEASE:=4

PKG_MAINTAINER:=sbwml <admin@cooluc.com>
PKG_LICENSE:=GPL-3.0-or-later

LUCI_TITLE:=LuCI Support for Disk Manager
LUCI_DESCRIPTION:=Provides Web UI for Disk Manager

LUCI_DEPENDS:=+luci-base +ucode +block-mount +parted +smartmontools \
	+dosfstools +e2fsprogs +exfat-fsck +exfat-mkfs +kmod-fs-ext4 +kmod-fs-vfat +kmod-fs-exfat

LUCI_PKGARCH:=all

include $(TOPDIR)/feeds/luci/luci.mk

# call BuildPackage - OpenWrt buildroot signature
