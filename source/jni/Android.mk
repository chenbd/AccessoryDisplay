LOCAL_PATH:= $(call my-dir)

#################################
include $(CLEAR_VARS)

LOCAL_SRC_FILES := com_android_accessorydisplay_source_ScreenRecord.cpp

LOCAL_C_INCLUDES:=      \
        frameworks/av/cmds/screenrecord

LOCAL_SHARED_LIBRARIES:= libandroid_runtime libutils liblog libcutils libnativehelper libandroid libscreenrecord-$(PLATFORM_SDK_VERSION)

LOCAL_PRELINK_MODULE := false
LOCAL_MODULE_TAGS := optional
LOCAL_REQUIRED_MODULES := libscreenrecord-$(PLATFORM_SDK_VERSION)

LOCAL_MODULE:= libscreenrecord_jni
include $(BUILD_SHARED_LIBRARY)
