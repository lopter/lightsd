#
# Copyright (C) 2015 Louis Opter <kalessin@kalessin.fr>
#
# This is free software, licensed under the GNU General Public License v2.
# See /LICENSE for more information.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=lightsd
PKG_VERSION:={{ version }}
PKG_RELEASE:={{ build_number }}
PKG_MAINTAINER:=Louis Opter <kalessin@kalessin.fr>
PKG_LICENSE:=GPL-3.0+
PKG_SOURCE_URL:=https://downloads.lightsd.io/releases/
PKG_SOURCE:={{ archive_name }}
PKG_MD5SUM:={{ archive_md5 }}
PKG_BUILD_DIR:=$(BUILD_DIR)/lightsd-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk
include $(INCLUDE_DIR)/cmake.mk

CMAKE_OPTIONS += \
	-DCMAKE_BUILD_TYPE=RELEASE \
	-DLGTD_RUNTIME_DIRECTORY=/var/run/lightsd

define Package/lightsd
  SECTION:=utils
  CATEGORY:=Utilities
  DEPENDS:=+libevent2-core
  TITLE:=Daemon to control your LIFX Wi-Fi smart bulbs
  MAINTAINER:=Louis Opter <kalessin@kalessin.fr>
  URL:=https://github.com/lopter/lightsd
  USERID:=lightsd=56700:lightsd=56700
endef

define Package/lightsd/install
	$(INSTALL_DIR) \
		$(1)/etc/init.d/ \
		$(1)/usr/bin \
		$(1)/usr/share/lightsd \
		$(1)/usr/share/doc/lightsd

	$(INSTALL_BIN) \
		$(PKG_INSTALL_DIR)/usr/bin/lightsd \
		$(1)/usr/bin/lightsd

	$(INSTALL_BIN) \
		./files/lightsd.init \
		$(1)/etc/init.d/lightsd

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/share/lightsd \
		$(1)/usr/share

	$(CP) \
		$(PKG_INSTALL_DIR)/usr/share/doc/lightsd \
		$(1)/usr/share/doc
endef

$(eval $(call BuildPackage,lightsd))

