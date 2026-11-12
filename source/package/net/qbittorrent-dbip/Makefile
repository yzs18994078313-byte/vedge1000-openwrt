include $(TOPDIR)/rules.mk

PKG_NAME:=qbittorrent-dbip
PKG_VERSION:=2026.11
PKG_RELEASE:=1
PKG_SOURCE:=dbip-country-lite-$(subst .,-,$(PKG_VERSION)).mmdb.gz
PKG_SOURCE_URL:=https://download.db-ip.com/free/
PKG_HASH:=75fe430010f6d38a8c4331c202bf0df6ce3221a6e6d823b9e5a6d951e4cf9758
PKG_LICENSE:=CC-BY-4.0

include $(INCLUDE_DIR)/package.mk

define Package/qbittorrent-dbip
  SECTION:=net
  CATEGORY:=Network
  TITLE:=Offline DB-IP Country Lite database for qBittorrent
  URL:=https://db-ip.com/db/lite.php
  PKGARCH:=all
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	gzip -dc $(DL_DIR)/$(PKG_SOURCE) > $(PKG_BUILD_DIR)/dbip-country-lite.mmdb
endef

define Build/Compile
endef

define Package/qbittorrent-dbip/install
	$(INSTALL_DIR) $(1)/usr/share/qbittorrent
	$(INSTALL_DATA) $(PKG_BUILD_DIR)/dbip-country-lite.mmdb $(1)/usr/share/qbittorrent/
	$(INSTALL_DATA) ./files/DB-IP-LICENSE.txt $(1)/usr/share/qbittorrent/
endef

$(eval $(call BuildPackage,qbittorrent-dbip))
