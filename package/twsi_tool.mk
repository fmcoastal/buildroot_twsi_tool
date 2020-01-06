################################################################################
#
# PACKAGE_NAME: twsi_tool
#
# the directives below are defined in:
#
# from https://groups.google.com/forum/#!topic/acmesystems/kPjYepcftLM
# 
################################################################################

TWSI_TOOL_VERSION = 1.0.0
TWSI_TOOL_SOURCE = twsi_tool-$(TWSI_TOOL_VERSION).tgz

#STRIP_COMPONENTS tell the tar how many directory structures to 
# strip off the front end of your archive. (See directive in manual)
TWSI_TOOL_STRIP_COMPONENTS=0
####  WHERE TO DOWNLOAD YOUR SOURCE FROM

#TWSI_TOOL_SITE_METHOD = wget
#TWSI_TOOL_SITE = root@10.11.65.84:80
TWSI_TOOL_SITE_METHOD = file
TWSI_TOOL_SITE = /scratch/twsi_tool


####  END Download Options

TWSI_TOOL_LICENSE = GPL-2.0
TWSI_TOOL_LICENSE_FILES = COPYING

TWSITEST_INSTALL_TARGET:=YES

#BUILD COMMAND LINE
#  if defined, buildroot will execute.
define TWSI_TOOL_BUILD_CMDS
        $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D) all
endef

#INSTALL COMMAND LINE - places executible in resulting /bin directory
#  if defined, buildroot will execute.
define TWSI_TOOL_INSTALL_TARGET_CMDS
        $(INSTALL) -D -m 0755 $(@D)/twsi_tool $(TARGET_DIR)/bin
endef

#HOW PERMISSIONS ARE SET
#  if defined, buildroot will execute.
define TWSI_TOOL_PERMISSIONS
       /bin/twsi_tool f 4755 0 0 - - - - - 
endef


$(eval $(generic-package))


