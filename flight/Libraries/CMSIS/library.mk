#
# Rules to add CMSIS to a target
#

CMSIS_DIR		:=	$(dir $(lastword $(MAKEFILE_LIST)))
EXTRAINCDIRS		+=	$(CMSIS_DIR)/Include
