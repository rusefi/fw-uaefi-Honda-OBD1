BOARDCPPSRC =  $(BOARD_DIR)/board_configuration.cpp \
    $(BOARD_DIR)/ext/rusefi/firmware/config/boards/hellen/uaefi121/mega-uaefi.cpp \


BOARDINC += $(BOARD_DIR)/generated/controllers/generated \
    $(BOARD_DIR)/ext/rusefi/firmware/config/boards/hellen/uaefi121/ \


# defines SHORT_BOARD_NAME
include $(BOARD_DIR)/meta-info.env

include $(BOARD_DIR)/ext/rusefi/firmware/config/boards/hellen/uaefi121/mega-uaefi.mk