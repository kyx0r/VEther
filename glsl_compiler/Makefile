ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
endif

CXX = g++
LIBNAME = glslang
LIBPATH = ../lib
DEFINES =  -DENABLE_OPT=0 -DENABLE_NV_EXTENSIONS -DENABLE_AMD_EXTENSIONS -DNV_EXTENSIONS
CXXFLAGS = $(SHARED_FLAGS) -fPIC -I.. -I ./OGLCompilersDLL $(DEFINES) -I ../glfw/include -I ../include

ifeq ($(detected_OS),Windows)
OSSOURCE = $(wildcard ./glslang/OSDependent/Windows/*.cpp)
DEFINES += -DFILEWATCHER_PLATFORM=FILEWATCHER_PLATFORM_WIN32
endif

ifeq ($(detected_OS),Linux)
OSSOURCE = $(wildcard ./glslang/OSDependent/Unix/*.cpp)
DEFINES += -DFILEWATCHER_PLATFORM=FILEWATCHER_PLATFORM_LINUX
endif

GLSLANG=$(wildcard ./glslang/MachineIndependent/*.cpp)
PREPROCESSOR = $(wildcard ./glslang/MachineIndependent/preprocessor/*.cpp)
STANDALONE=$(wildcard ./StandAlone/*.cpp)
SPIRV = $(wildcard ./SPIRV/*.cpp)
OGLCompilersDLL=$(wildcard ./OGLCompilersDLL/*.cpp)
FileWatcher=$(wildcard ./SimpleFileWatcher/*.cpp)

OBJS=$(OGLCompilersDLL:.cpp=.o)
OBJS2=$(OSSOURCE:.cpp=.o)
OBJS3=$(GLSLANG:.cpp=.o)
OBJS4=$(PREPROCESSOR:.cpp=.o)
OBJS5=$(STANDALONE:.cpp=.o)
OBJS6=$(SPIRV:.cpp=.o)
OBJS7=$(FileWatcher:.cpp=.o)

all: $(LIBPATH)/lib$(LIBNAME).a

clean:
	rm -f $(OBJS) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6) $(OBJS7) lib$(LIBNAME).a
	rm -rf ../lib/lib$(LIBNAME).a

lib$(LIBNAME).a: $(OBJS) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6) $(OBJS7)
	ar -cvr lib$(LIBNAME).a $(OBJS) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6) $(OBJS7)
	gcc-ranlib lib$(LIBNAME).a

.PHONY $(LIBPATH):
	mkdir -p ../lib

$(LIBPATH)/lib$(LIBNAME).a: lib$(LIBNAME).a $(LIBPATH)
	cp -d lib$(LIBNAME).a ../lib
