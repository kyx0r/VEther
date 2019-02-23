ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
endif

ifeq ($(detected_OS),Windows)
OGLCompilersDLL=$(wildcard ./OGLCompilersDLL/*.cpp)
OSSOURCE = $(wildcard ./glslang/OSDependent/Windows/*.cpp)
endif

ifeq ($(detected_OS),Linux)
OSSOURCE = $(wildcard ./glslang/OSDependent/Unix/*.cpp)
endif

GLSLANG=$(wildcard ./glslang/MachineIndependent/*.cpp)
PREPROCESSOR = $(wildcard ./glslang/MachineIndependent/preprocessor/*.cpp)
STANDALONE=$(wildcard ./StandAlone/*.cpp)
SPIRV = $(wildcard ./SPIRV/*.cpp)

OBJS=$(OGLCompilersDLL:.cpp=.o)
OBJS2=$(OSSOURCE:.cpp=.o)
OBJS3=$(GLSLANG:.cpp=.o)
OBJS4=$(PREPROCESSOR:.cpp=.o)
OBJS5=$(STANDALONE:.cpp=.o)
OBJS6=$(SPIRV:.cpp=.o)

CC = g++
LIBNAME = glslang
LIBPATH = ../lib
DEFINES =  -DENABLE_OPT=0 -DENABLE_NV_EXTENSIONS -DENABLE_AMD_EXTENSIONS -DNV_EXTENSIONS
CXXFLAGS = $(SHARED_FLAGS) -fPIC -I.. -I ./OGLCompilersDLL $(DEFINES)

all: $(LIBPATH)/lib$(LIBNAME).a

clean:
	rm -f $(OBJS) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6) lib$(LIBNAME).a
	rm -rf ../lib/lib$(LIBNAME).a

lib$(LIBNAME).a: $(OBJS) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6)
	ar -cvr lib$(LIBNAME).a $(OBJS) $(OBJS2) $(OBJS3) $(OBJS4) $(OBJS5) $(OBJS6)
	ranlib lib$(LIBNAME).a

.PHONY $(LIBPATH):
	mkdir -p ../lib

$(LIBPATH)/lib$(LIBNAME).a: lib$(LIBNAME).a $(LIBPATH)
	cp -d lib$(LIBNAME).a ../lib