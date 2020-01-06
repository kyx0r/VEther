ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
endif

#OBJS specifies which files to compile as part of the project 
OBJS = ./*cpp ./src/*o 
ifeq ($(detected_OS),Windows)
endif

#CC specifies which compiler we're using 
CC = g++
ASTYLE = astyle

SRCS=$(wildcard ./src/*.cpp)
PROGS = $(patsubst ./src/%.cpp, ./build/%.o,$(SRCS))

.DEFAULT_GOAL := default

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

VETHER = -lVEther -lglfw -lglslang -lbullet3
 
#OBJ_NAME specifies the name of our exectuable 
OBJ_NAME = vether.exe 

#for further optimization use -flto flag.
SHARED_FLAGS = -O3 -g -Wl,--no-relax -m64 -Wall -Wextra -masm=intel -fno-align-functions -fno-exceptions -Wno-deprecated-copy -Wno-unused-parameter -Wno-cast-function-type -Wno-write-strings
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
	$(MAKE) all -C ./bullet3
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
	$(MAKE) all -C ./bullet3
	$(MAKE) all -C ./src
	$(CC) -flto -static main.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(VETHER) $(SHARED_FLAGS) $(LINKER_FLAGS) $(WINAPI) -o $(OBJ_NAME)

#all_sl <- any other system.	
all_sl: VEther
	$(CC) main.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(VETHER) $(SHARED_FLAGS) $(_UNIX) -o $(OBJ_NAME)	

all_u:
	$(MAKE) all -C ./glsl_compiler
	$(MAKE) all -C ./glfw
	$(MAKE) all -C ./bullet3
	$(CC) -c $(SHARED_FLAGS) $(INCLUDE_PATHS) -I ./bullet3 -I ./src -o ./lib/vether.o vether.cpp 
	ar -cvr ./lib/libVEther.a ./lib/vether.o
	gcc-ranlib ./lib/libVEther.a
	$(CC) main.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(VETHER) $(SHARED_FLAGS) $(_UNIX) -o $(OBJ_NAME)	

debug:
	objcopy --only-keep-debug $(OBJ_NAME) main.debug

clean_f:
	find . -type f -name '*.orig' -delete

clean_o:
	$(MAKE) clean -C ./glsl_compiler
	$(MAKE) clean -C ./src
	$(MAKE) clean -C ./glfw
	$(MAKE) clean -C ./bullet3
	find . -type f -name '*.o' -delete

c:
	$(MAKE) clean -C ./src

.IGNORE format:
	$(ASTYLE) --style=allman --indent=tab --recursive *.c, *.cpp, *.h, *.hpp
etags:
	 find . \( -name "*[tT]est*" -o -name "CVS" -o -name "*#*" -o -name "html" -o -name "*~" -o -name "*.ca*" \) -prune -o \( -iname "*.c" -o -iname "*.cpp" -o -iname "*.cxx" -o -iname "*.h"  -o -iname "*.hh" \) -exec etags -a {} \;

.SILENT default:
	echo "Please specify the target."
	echo "all_slwin = incremental build for windows"
	echo "all_flto = 32 bit lto build for windows"
	echo "all_sl = incrementail build for *nix"
	echo "all_u = uniform build for *nix"
