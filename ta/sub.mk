global-incdirs-y += include
srcs-y += trust_chain_ta.c
srcs-y += key_list/key_list.c
srcs-y += utils/utils.c
srcs-y += tee_key_manager/tee_key_manager.c
srcs-y += block/block.c

# To remove a certain compiler flag, add a line like this
#cflags-template_ta.c-y += -Wno-strict-prototypes
