BOARDCPPSRC =  $(BOARD_DIR)/board_configuration.cpp \
    $(BOARD_DIR)/ext/rusefi/firmware/config/boards/hellen/uaefi121/mega-uaefi.cpp \


BOARDINC += $(BOARD_DIR)/generated/controllers/generated \
    $(BOARD_DIR)/ext/rusefi/firmware/config/boards/hellen/uaefi121/ \


DDEFS += -DDEFAULT_ENGINE_TYPE=engine_type_e::HONDA_OBD1

# defines SHORT_BOARD_NAME
include $(BOARD_DIR)/meta-info.env

include $(BOARD_DIR)/ext/rusefi/firmware/config/boards/hellen/uaefi121/mega-uaefi.mk