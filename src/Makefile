
SRCS=$(wildcard *.cpp)

OBJS=$(SRCS:.cpp=.o)

CC = g++
LIBNAME = VEther
LIBPATH = ../lib
CXXFLAGS = -O3 -m64 -s -Wall -fPIC -DPIC -I ../include -I ../glfw/include -Wextra -masm=intel -mpush-args -mno-accumulate-outgoing-args -mno-stack-arg-probe 

all: $(LIBPATH)/lib$(LIBNAME).a

clean:
	rm -f $(OBJS) lib$(LIBNAME).a
	rm -rf $(LIBPATH)/lib$(LIBNAME).a

lib$(LIBNAME).a: $(OBJS)
	ar -cvr lib$(LIBNAME).a $(OBJS)
	ranlib lib$(LIBNAME).a

$(LIBPATH):
	mkdir -p ../lib

$(LIBPATH)/lib$(LIBNAME).a: lib$(LIBNAME).a $(LIBPATH)
	cp -d lib$(LIBNAME).a ../lib