include $(TOPDIR)/rules.mk

PKG_NAME:=vedge-usb-automount
PKG_VERSION:=1.0
PKG_RELEASE:=2
PKG_LICENSE:=GPL-2.0-only

include $(INCLUDE_DIR)/package.mk

define Package/vedge-usb-automount
  SECTION:=utils
  CATEGORY:=Utilities
  TITLE:=Automatic USB disk mounting for vEdge 1000
  DEPENDS:=@TARGET_octeon +blkid +block-mount +flock +qbittorrent-nox +kmod-fs-ext4 +kmod-fs-exfat +kmod-fs-vfat
  PKGARCH:=all
endef

define Build/Compile
endef

define Package/vedge-usb-automount/install
	$(INSTALL_DIR) $(1)/usr/libexec $(1)/etc/init.d $(1)/etc/hotplug.d/block $(1)/etc/uci-defaults
	$(INSTALL_BIN) ./files/vedge-usb-automount $(1)/usr/libexec/
	$(INSTALL_BIN) ./files/vedge-usb-automount.init $(1)/etc/init.d/vedge-usb-automount
	$(INSTALL_DATA) ./files/90-vedge-usb-automount $(1)/etc/hotplug.d/block/
	$(INSTALL_BIN) ./files/09-vedge-usb-automount-fstab $(1)/etc/uci-defaults/
endef

$(eval $(call BuildPackage,vedge-usb-automount))
