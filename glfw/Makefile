ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
endif

LIBFLAGS = -fPIC

#Note: This is low level setup. Its up to you to choose which libraries to build!
#Here is the lists presented, but does not gurantee it would work off the bat.
#If you too lazy to figure this things out, then go and get the library distro with CMAKE then!
SRCS=$(wildcard ./deps/*.c)
ifeq ($(detected_OS),Windows)
SRCS2 = ./src/xkb_unicode.c ./src/window.c ./src/win32_window.c ./src/win32_time.c ./src/win32_thread.c  
SRCS2 += ./src/win32_monitor.c  ./src/win32_init.c  ./src/wgl_context.c ./src/vulkan.c ./src/egl_context.c
SRCS2 += ./src/osmesa_context.c ./src/monitor.c ./src/input.c ./src/init.c ./src/context.c ./src/win32_joystick.c
LIBFLAGS += -D_GLFW_WIN32
endif

#Assuming that X11 driver is present!
#get xorg-dev package if its not present
ifeq ($(detected_OS),Linux)
SRCS2 = ./src/xkb_unicode.c ./src/window.c ./src/posix_time.c ./src/posix_thread.c 
SRCS +=  ./src/glx_context.c ./src/egl_context.c ./src/linux_joystick.c
SRCS2 += ./src/vulkan.c ./src/x11_init.c ./src/x11_monitor.c ./src/x11_window.c
SRCS2 += ./src/osmesa_context.c ./src/monitor.c ./src/input.c ./src/init.c ./src/context.c
LIBFLAGS += -D_GLFW_X11
else
#SRCS2=$(wildcard ./src/*.c)
endif

#./src/win32_joystick.c ./src/posix_time.c ./src/posix_thread.c ./src/null_window.c  ./src/null_monitor.c  ./src/null_joystick.c  ./src/null_init.c ./src/glx_context.c

OBJS=$(SRCS:.c=.o)
OBJS2=$(SRCS2:.c=.o)

CC = gcc

LIBNAME = glfw
LIBPATH = ../lib
CFLAGS = $(LIBFLAGS) $(SHARED_FLAGS) -I.. -I ./deps -I ../include

all: $(LIBPATH)/lib$(LIBNAME).a

clean:
	rm -f $(OBJS) $(OBJS2) $(OBJS3) lib$(LIBNAME).a
	rm -rf ../lib/lib$(LIBNAME).a

lib$(LIBNAME).a: $(OBJS) $(OBJS2) $(OBJS3)
	ar -cvr lib$(LIBNAME).a $(OBJS) $(OBJS2) $(OBJS3)
	gcc-ranlib lib$(LIBNAME).a

.PHONY $(LIBPATH):
	mkdir -p ../lib

$(LIBPATH)/lib$(LIBNAME).a: lib$(LIBNAME).a $(LIBPATH)
	cp -d lib$(LIBNAME).a ../lib
