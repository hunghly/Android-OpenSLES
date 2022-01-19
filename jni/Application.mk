APP_OPTIM := debug    # Build the target in debug mode. 
APP_ABI := x86 # Define the target architecture to be ARM.
APP_STL := c++_static
APP_CPPFLAGS := -frtti -fexceptions \
-std=c++11   # This is the place you enable exception.
APP_PLATFORM := android-22 # Define the target Android version of the native application.