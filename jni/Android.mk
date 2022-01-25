LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_C_INCLUDES:= $(LOCAL_PATH)/../include
LOCAL_SRC_FILES:= ../src/opensl.cpp ../src/main.cpp
LOCAL_MODULE:= opensl-recorder # The name of the binary.
LOCAL_LDLIBS:= -llog -lOpenSLES
include $(BUILD_EXECUTABLE)