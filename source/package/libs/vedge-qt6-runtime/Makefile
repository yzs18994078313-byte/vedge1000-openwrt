include $(TOPDIR)/rules.mk

PKG_NAME:=vedge-qt6-runtime
PKG_VERSION:=6.10.2
PKG_RELEASE:=1
PKG_SOURCE:=qtbase-everywhere-src-$(PKG_VERSION).tar.xz
PKG_SOURCE_URL:=https://download.qt.io/archive/qt/6.10/$(PKG_VERSION)/submodules/
PKG_HASH:=aeb78d29291a2b5fd53cb55950f8f5065b4978c25fb1d77f627d695ab9adf21e
PKG_BUILD_DIR:=$(BUILD_DIR)/qtbase-everywhere-src-$(PKG_VERSION)
PKG_BUILD_PARALLEL:=1
PKG_BUILD_FLAGS:=no-mips16
PKG_LICENSE:=LGPL-3.0-only
PKG_LICENSE_FILES:=LICENSES/LGPL-3.0-only.txt
CMAKE_INSTALL:=1

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

QT6_HOST:=$(TOPDIR)/../qbittorrent-development/host-qt/root/usr

define Package/vedge-qt6-runtime
  SECTION:=libs
  CATEGORY:=Libraries
  TITLE:=Qt 6 headless runtime for vEdge 1000
  DEPENDS:=@TARGET_octeon +libstdcpp +libatomic +zlib +libopenssl \
    +libpcre2-16
endef

# Headless subset of the Qt Base configuration used by ImmortalWrt.
CMAKE_OPTIONS += \
	-DBUILD_SHARED_LIBS=ON \
	-DQT_BUILD_EXAMPLES=OFF \
	-DQT_BUILD_TESTS=OFF \
	-DQT_BUILD_TOOLS_WHEN_CROSSCOMPILING=OFF \
	-DQT_HOST_PATH=$(QT6_HOST) \
	-DQT_HOST_PATH_CMAKE_DIR=$(QT6_HOST)/lib/x86_64-linux-gnu/cmake \
	-DINSTALL_PLUGINSDIR=lib/qt6plugins \
	-DFEATURE_gui=OFF \
	-DFEATURE_widgets=OFF \
	-DFEATURE_dbus=OFF \
	-DFEATURE_glib=OFF \
	-DFEATURE_icu=OFF \
	-DFEATURE_liburing=OFF \
	-DFEATURE_testlib=OFF \
	-DFEATURE_network=ON \
	-DFEATURE_gssapi=OFF \
	-DFEATURE_libproxy=OFF \
	-DFEATURE_openssl=ON \
	-DFEATURE_openssl_runtime=ON \
	-DFEATURE_xml=ON \
	-DFEATURE_sql=ON \
	-DFEATURE_sql_sqlite=ON \
	-DFEATURE_system_sqlite=OFF \
	-DFEATURE_system_doubleconversion=OFF \
	-DFEATURE_system_pcre2=ON \
	-DFEATURE_system_zlib=ON \
	-DFEATURE_brotli=OFF \
	-DFEATURE_zstd=OFF

define Package/vedge-qt6-runtime/install
	$(INSTALL_DIR) $(1)/usr/lib $(1)/usr/lib/qt6plugins/tls $(1)/usr/lib/qt6plugins/sqldrivers
	$(CP) $(foreach lib,Core Network Sql Xml,$(PKG_INSTALL_DIR)/usr/lib/libQt6$(lib).so.*) $(1)/usr/lib/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/qt6plugins/tls/libqopensslbackend.so $(1)/usr/lib/qt6plugins/tls/
	$(INSTALL_BIN) $(PKG_INSTALL_DIR)/usr/lib/qt6plugins/sqldrivers/libqsqlite.so $(1)/usr/lib/qt6plugins/sqldrivers/
endef

$(eval $(call BuildPackage,vedge-qt6-runtime))
