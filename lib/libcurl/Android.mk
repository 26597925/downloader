LOCAL_PATH:= $(call my-dir)

common_CFLAGS := -Wpointer-arith -Wwrite-strings -Wunused -Winline -Wnested-externs -Wmissing-declarations -Wmissing-prototypes -Wno-long-long -Wfloat-equal -Wno-multichar -Wsign-compare -Wno-format-nonliteral -Wendif-labels -Wstrict-prototypes -Wdeclaration-after-statement -Wno-system-headers -DHAVE_CONFIG_H

include $(CLEAR_VARS)
include $(LOCAL_PATH)/lib/Makefile.inc
CURL_HEADERS := \
	curlbuild.h \
	curl.h \
	curlrules.h \
	curlver.h \
	easy.h \
	mprintf.h \
	multi.h \
	stdcheaders.h \
	typecheck-gcc.h
LOCAL_ARM_MODE := arm
LOCAL_SRC_FILES := $(addprefix lib/,$(CSOURCES))
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include/
LOCAL_CFLAGS += $(common_CFLAGS) -DHAVE_ZLIB_H -DHAVE_ZLIB -DHAVE_LIBZ

LOCAL_C_INCLUDES += $(ANDROID_BUILD_TOP)/external/openssl/include/  \
                                     $(ANDROID_BUILD_TOP)/external/zlib

LOCAL_COPY_HEADERS_TO := libcurl/curl
LOCAL_COPY_HEADERS := $(addprefix include/curl/,$(CURL_HEADERS))
LOCAL_SHARED_LIBRARIES +=libssl libcrypto libz
LOCAL_SHARED_LIBRARIES += liblog libcutils
LOCAL_MODULE:= libcurl
LOCAL_MODULE_TAGS := optional

#ALL_PREBUILT += $(LOCAL_PATH)/NOTICE
$(LOCAL_PATH)/NOTICE: $(LOCAL_PATH)/COPYING | $(ACP)
	$(copy-file-to-target)
LOCAL_PRELINK_MODULE := false

include $(BUILD_STATIC_LIBRARY)


#########################
# Build the curl binary

include $(CLEAR_VARS)
include $(LOCAL_PATH)/src/Makefile.inc
LOCAL_SRC_FILES := $(addprefix src/,$(CURL_CFILES))

LOCAL_MODULE := curl
LOCAL_MODULE_TAGS := tests
#LOCAL_SHARED_LIBRARIES := libcurl
LOCAL_SYSTEM_SHARED_LIBRARIES := libcurl libc 
LOCAL_SHARED_LIBRARIES := liblog libcutils
LOCAL_C_INCLUDES += $(LOCAL_PATH)/include $(LOCAL_PATH)/lib

# This may also need to include $(CURLX_ONES) in order to correctly link
# if libcurl is changed to be built as a dynamic library
LOCAL_CFLAGS += $(common_CFLAGS)

include $(BUILD_EXECUTABLE)

