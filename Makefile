ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
endif

#OBJS specifies which files to compile as part of the project 
OBJS = ./*cpp ./src/*o 
ifeq ($(detected_OS),Windows)
# GP = ./glfw/src/
# OBJS += $(GP)xkb_unicode.o $(GP)window.o $(GP)win32_window.o $(GP)win32_time.o $(GP)win32_thread.o  
# OBJS += $(GP)win32_monitor.o $(GP)win32_init.o  $(GP)wgl_context.o $(GP)vulkan.o $(GP)egl_context.o
# OBJS += $(GP)osmesa_context.o $(GP)monitor.o $(GP)input.o $(GP)init.o $(GP)context.o $(GP)win32_joystick.o
# OBJS += ./glsl_compiler/OGLCompilersDLL/*.o
# OBJS += ./glsl_compiler/glslang/OSDependent/Windows/*.o
# OBJS += ./glsl_compiler/glslang/MachineIndependent/*.o
# OBJS += ./glsl_compiler/glslang/MachineIndependent/preprocessor/*.o
# OBJS += ./glsl_compiler/StandAlone/*.o
# OBJS += ./glsl_compiler/SPIRV/*.o
endif

#CC specifies which compiler we're using 
CC = g++

ASTYLE = astyle

SRCS=$(wildcard ./src/*.cpp)
PROGS = $(patsubst ./src/%.cpp, ./build/%.o,$(SRCS))

# -w suppresses all warnings 
# -Wl,-subsystem,windows gets rid of the console window 
#COMPILER_FLAGS = -fpermissive -m64 -s -Wall

LIBRARY_PATHS = -L ./lib

INCLUDE_PATHS = -I ./include -I ./glfw/include -I .

#LINKER_FLAGS specifies the libraries we're linking against 
LINKER_FLAGS = -static-libgcc -static-libstdc++ 
#-lpthread

WINAPI = -lmingw32 -lkernel32 -lm -ldxguid -ldxerr8 -luser32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lgdi32 -lcomdlg32 -lwinspool 
WINAPI+= -lcomctl32 -luuid -lrpcrt4 -ladvapi32 -lwsock32 -lshlwapi -lversion -ldbghelp -lwinpthread

_UNIX = -lpthread -lX11 -lxcb -lXau -lXext -lXdmcp -lpthread -ldl

VETHER = -lVEther -lglfw -lglslang
 
#OBJ_NAME specifies the name of our exectuable 
OBJ_NAME = VEther.exe 

#for further optimization use -flto flag.
SHARED_FLAGS = -O3 -g -m64 -Wall -Wextra -masm=intel -fno-align-functions -fno-exceptions -Wno-deprecated-copy -Wno-unused-parameter -Wno-cast-function-type -Wno-write-strings
export SHARED_FLAGS
#-mpush-args -mno-accumulate-outgoing-args -mno-stack-arg-probe

#Monolithic compile.
all : 
	$(CC) -Os -static $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(SHARED_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
	
allwin : 
	$(CC) -Os -static $(OBJS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(SHARED_FLAGS) $(LINKER_FLAGS) $(WINAPI) -o $(OBJ_NAME)	

all_individual : $(PROGS)
./build/%.o: ./src/%.cpp
	$(CC) -c -static $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(SHARED_FLAGS) $(LINKER_FLAGS) $(WINAPI) -o $@ $<
	
VEther: 
	$(MAKE) all -C ./glsl_compiler
	$(MAKE) all -C ./glfw
	$(MAKE) all -C ./src
	
#Building a static lib out of src files. Benefits - faster compile time. Makes project modular.
#all_slwin <- use this for compilation on windows OS.
all_slwin: VEther
	$(CC) -flto -static main.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(VETHER) $(SHARED_FLAGS) $(LINKER_FLAGS) $(WINAPI) -o $(OBJ_NAME)
	
glsl_m32: SHARED_FLAGS = -Os -m32 -s -Wall -Wextra -fno-align-functions -fno-exceptions -Wno-unused-parameter -Wno-cast-function-type -Wno-write-strings	
	
glsl_m32: 
	$(MAKE) all -C ./glsl_compiler
	
all_flto: SHARED_FLAGS = -flto -O3 -m32 -s -Wall -Wextra -fno-align-functions -fno-exceptions -Wno-unused-parameter -Wno-cast-function-type -Wno-write-strings

all_flto: glsl_m32
	$(MAKE) all -C ./glfw
	$(MAKE) all -C ./src
	$(CC) -flto -static main.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(VETHER) $(SHARED_FLAGS) $(LINKER_FLAGS) $(WINAPI) -o $(OBJ_NAME)

#all_sl <- any other system.	
all_sl: VEther
	$(CC) -O3 main.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(VETHER) $(SHARED_FLAGS) $(_UNIX) -o $(OBJ_NAME)	
	
debug:
	objcopy --only-keep-debug $(OBJ_NAME) main.debug
	
clean_f:
	find . -type f -name '*.orig' -delete
	
clean_o:
	$(MAKE) clean -C ./glsl_compiler
	$(MAKE) clean -C ./src
	$(MAKE) clean -C ./glfw
	find . -type f -name '*.o' -delete
	
c:
	$(MAKE) clean -C ./src
	
.IGNORE format:
	$(ASTYLE) --style=allman --indent=tab ./*.cpp, *.h
	$(ASTYLE) --style=allman --indent=tab ./src/*.cpp, *.h
	$(ASTYLE) --style=allman --indent=tab --recursive ./src/*.cpp, *.h
	
	
	
	
	
