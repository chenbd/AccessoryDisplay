# Copyright (C) 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

LOCAL_PATH := $(call my-dir)

# Build the application.
include $(CLEAR_VARS)
LOCAL_PACKAGE_NAME := AccessoryDisplaySource

# frameworks/base/core/res/AndroidManifest.xml
#    <!-- Allows an application to use SurfaceFlinger's low level features.
#    <p>Not for use by third-party applications. -->
#    <permission android:name="android.permission.ACCESS_SURFACE_FLINGER"
#    android:label="@string/permlab_accessSurfaceFlinger"
#    android:description="@string/permdesc_accessSurfaceFlinger"
#    android:protectionLevel="signature" />
LOCAL_CERTIFICATE := platform

LOCAL_MODULE_TAGS := tests
LOCAL_SDK_VERSION := current
LOCAL_SRC_FILES := $(call all-java-files-under, src)
LOCAL_RESOURCE_DIR = $(LOCAL_PATH)/res
LOCAL_STATIC_JAVA_LIBRARIES := AccessoryDisplayCommon
include $(BUILD_PACKAGE)

include $(call all-makefiles-under,$(LOCAL_PATH)/jni)
