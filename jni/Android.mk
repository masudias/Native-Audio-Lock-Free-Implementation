LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE   := opensl_example
LOCAL_C_INCLUDES := $(LOCAL_PATH)
LOCAL_CFLAGS := -O3 
LOCAL_CPPFLAGS :=$(LOCAL_CFLAGS)
###

LOCAL_SRC_FILES := opensl_example.c \
	opensl_io.c \
	java_interface_wrap.cpp \
	basop.c \
	cod_cng.c \
	coder.c \
	dec_cng.c \
	decod.c \
	exc_lbc.c \
	lbccodec.c \
	lpc.c \
	lsp.c \
	tab_lbc.c \
	tame.c \
	util_cng.c \
	util_lbc.c \
	vad.c \

LOCAL_LDLIBS := -llog -lOpenSLES

include $(BUILD_SHARED_LIBRARY)


