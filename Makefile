# Else exist specifically for clang
ifeq ($(CXX),g++)
    EXTRA_FLAGS = --no-gnu-unique
else
    EXTRA_FLAGS =
endif

all:
	$(CXX) -shared -fPIC $(EXTRA_FLAGS) -Iinclude src/main.cpp src/Hyprgrid.cpp src/config.cpp src/dispatcher.cpp -o myplugin.so -g `pkg-config --cflags pixman-1 libdrm hyprland pangocairo libinput libudev wayland-server xkbcommon` -std=c++2b -Wno-narrowing
clean:
	rm -f ./myplugin.so
