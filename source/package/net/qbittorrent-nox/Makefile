include $(TOPDIR)/rules.mk

PKG_NAME:=qbittorrent-nox
PKG_VERSION:=5.3.0
PKG_RELEASE:=8
PKG_SOURCE:=qbittorrent-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://github.com/qbittorrent/qBittorrent/releases/download/release-$(PKG_VERSION)/
PKG_HASH:=9ea9c0256f976921abd8e0294241464af9977790255d97a186af76d3f0d67a7e
PKG_BUILD_DIR:=$(BUILD_DIR)/qbittorrent-$(PKG_VERSION)
PKG_BUILD_DEPENDS:=boost
PKG_BUILD_PARALLEL:=1
PKG_BUILD_FLAGS:=no-mips16
PKG_LICENSE:=GPL-2.0-or-later
PKG_LICENSE_FILES:=COPYING
CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

QT6_HOST:=$(TOPDIR)/../qbittorrent-development/host-qt/root/usr

define Package/qbittorrent-nox
  SECTION:=net
  CATEGORY:=Network
  SUBMENU:=BitTorrent
  TITLE:=qBittorrent headless BitTorrent client
  URL:=https://www.qbittorrent.org/
  DEPENDS:=@TARGET_octeon +vedge-qt6-runtime +libtorrent-rasterbar +libopenssl +zlib +qbittorrent-dbip
  USERID:=qbittorrent:qbittorrent
endef

define Package/qbittorrent-nox/conffiles
/etc/config/qbittorrent
/etc/qBittorrent/
endef

CMAKE_OPTIONS += \
	-DGUI=OFF \
	-DWEBUI=ON \
	-DDBUS=OFF \
	-DPLUGINS=OFF \
	-DSTACKTRACE=OFF \
	-DSYSTEMD=OFF \
	-DQBT_VER_STATUS= \
	-DQt6LinguistTools_DIR=$(QT6_HOST)/lib/x86_64-linux-gnu/cmake/Qt6LinguistTools \
	-DQT_ADDITIONAL_PACKAGES_PREFIX_PATH=$(QT6_HOST)

define Package/qbittorrent-nox/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/usr/libexec $(1)/etc/config $(1)/etc/init.d $(1)/etc/uci-defaults
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/bin/qbittorrent-nox $(1)/usr/bin/
	$(INSTALL_BIN) ./files/qbittorrent-start $(1)/usr/libexec/qbittorrent-start
	$(INSTALL_BIN) ./files/qbittorrent.init $(1)/etc/init.d/qbittorrent
	$(INSTALL_CONF) ./files/qbittorrent.config $(1)/etc/config/qbittorrent
	$(INSTALL_BIN) ./files/60_qbittorrent_user $(1)/etc/uci-defaults/
	$(INSTALL_BIN) ./files/61_qbittorrent_profile_owner $(1)/etc/uci-defaults/
endef

$(eval $(call BuildPackage,qbittorrent-nox))
