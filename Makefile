ifeq ($(OS),Windows_NT)
    detected_OS := Windows
else
    detected_OS := $(shell sh -c 'uname -s 2>/dev/null || echo not')
endif

#OBJS specifies which files to compile as part of the project 
OBJS = ./*cpp ./src/*cpp
#HEADS = ./*.h

#CC specifies which compiler we're using 
CC = g++

ASTYLE = aStyle

SRCS=$(wildcard ./src/*.cpp)
PROGS = $(patsubst ./src/%.cpp, ./build/%.o,$(SRCS))

# -w suppresses all warnings 
# -Wl,-subsystem,windows gets rid of the console window 
#COMPILER_FLAGS = -fpermissive -m64 -s -Wall

LIBRARY_PATHS = -L ./lib

INCLUDE_PATHS = -I ./include -I ./glfw/include

#LINKER_FLAGS specifies the libraries we're linking against 
LINKER_FLAGS = -static-libgcc -static-libstdc++ 
#-lpthread

WINAPI = -lmingw32 -lkernel32 -lm -ldxguid -ldxerr8 -luser32 -lwinmm -limm32 -lole32 -loleaut32 -lshell32 -lgdi32 -lcomdlg32 -lwinspool 
WINAPI+= -lcomctl32 -luuid -lrpcrt4 -ladvapi32 -lwsock32 -lshlwapi -lversion -ldbghelp -lwinpthread

VETHER = -lVEther -lglfw
 
#OBJ_NAME specifies the name of our exectuable 
OBJ_NAME = VEther.exe 

SHARED_FLAGS = -O3 -m64 -s -Wall -Wextra -Wno-unused-parameter -Wno-cast-function-type
export
#-mpush-args -mno-accumulate-outgoing-args -mno-stack-arg-probe

#Monolithic compile.
all : 
	$(CC) -O0 -static $(OBJS) $(HEADS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(SHARED_FLAGS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)
	
allwin : 
	$(CC) -O0 -static $(OBJS) $(HEADS) $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(SHARED_FLAGS) $(COMPILER_FLAGS) $(LINKER_FLAGS) $(WINAPI) -o $(OBJ_NAME)	

all_individual : $(PROGS)
./build/%.o: ./src/%.cpp
	$(CC) -c -static $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(COMPILER_FLAGS) $(SHARED_FLAGS) $(COMPILER_FLAGS) $(LINKER_FLAGS) $(WINAPI) -o $@ $<
	
VEther: 
	$(MAKE) all -C ./glfw
	$(MAKE) all -C ./src
	
#Building a static lib out of src files. Benefits - faster compile time. Makes project modular.
#all_slwin <- use this for compilation on windows OS.
all_slwin: VEther
	$(CC) -O3 -static main.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(VETHER) $(SHARED_FLAGS) $(COMPILER_FLAGS) $(LINKER_FLAGS) $(WINAPI) -o $(OBJ_NAME)

#all_sl <- any other system.	
all_sl: VEther
	$(CC) -O3 -static main.cpp $(INCLUDE_PATHS) $(LIBRARY_PATHS) $(VETHER) $(SHARED_FLAGS) $(COMPILER_FLAGS) $(LINKER_FLAGS) -o $(OBJ_NAME)	
	
clean_f:
	find . -type f -name '*.orig' -delete
	
clean_o:
	$(MAKE) clean -C ./src
	$(MAKE) clean -C ./glfw
	find . -type f -name '*.o' -delete
	
.IGNORE format:
	$(ASTYLE) --style=allman --indent=tab ./*.cpp, *.h
	$(ASTYLE) --style=allman --indent=tab ./src/*.cpp, *.h
	$(ASTYLE) --style=allman --indent=tab --recursive ./src/*.cpp, *.h
	
	
	
	
	