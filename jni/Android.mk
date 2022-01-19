LOCAL_PATH:= $(call my-dir) # Get the local path of the project.
include $(CLEAR_VARS) # Clear all the variables with a prefix "LOCAL_"
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/../include
LOCAL_SRC_FILES:= ../src/test-audio.cpp
LOCAL_MODULE:= test-audio # The name of the binary.
LOCAL_LDLIBS:= -llog -lOpenSLES
include $(BUILD_EXECUTABLE) # Tell ndk-build that we want to build a native executable.