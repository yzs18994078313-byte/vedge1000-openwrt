include $(TOPDIR)/rules.mk
include $(INCLUDE_DIR)/kernel.mk

PKG_NAME:=octeon-aes-cbc
PKG_RELEASE:=1
PKG_LICENSE:=GPL-2.0-only
PKG_BUILD_DIR:=$(KERNEL_BUILD_DIR)/$(PKG_NAME)

include $(INCLUDE_DIR)/package.mk

define KernelPackage/crypto-aes-cbc-octeon
  SUBMENU:=Cryptographic API modules
  TITLE:=Octeon COP2 AES-CBC
  DEPENDS:=@TARGET_octeon +kmod-crypto-cbc
  FILES:=$(PKG_BUILD_DIR)/octeon-aes-cbc.ko
  AUTOLOAD:=$(call AutoLoad,15,octeon-aes-cbc)
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/. $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(KERNEL_MAKE) M="$(PKG_BUILD_DIR)" modules
endef

$(eval $(call KernelPackage,crypto-aes-cbc-octeon))
