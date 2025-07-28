CFG_TEE_TA_LOG_LEVEL ?= 4
CFG_TA_OPTEE_CORE_API_COMPAT_1_1=y

$(info TRUST_CHAIN_TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR))


# Enable user TA header defines generation
# CFG_TA_USER_TA_HEADER_DEFINES=y

# The UUID for the Trusted Application
BINARY=8aaaf201-2450-11e4-abe2-0002a5d5c51b

# # Add key_list source files
# SRCS += key_list/key_list.c

# # Add utils source files
# SRCS += utils/utils.c

# # Add tee_key_manager source files
# SRCS += tee_key_manager/tee_key_manager.c

# # Add block source files
# SRCS += block/block.c

-include $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk

ifeq ($(wildcard $(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk), )
clean:
	@echo 'Note: $$(TA_DEV_KIT_DIR)/mk/ta_dev_kit.mk not found, cannot clean TA'
	@echo 'Note: TA_DEV_KIT_DIR=$(TA_DEV_KIT_DIR)'
endif
 