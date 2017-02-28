#
# Rules to add CMSIS to a PiOS target
#

CMSIS_DIR		:=	$(dir $(lastword $(MAKEFILE_LIST)))
EXTRAINCDIRS		+=	$(CMSIS_DIR)/Include
