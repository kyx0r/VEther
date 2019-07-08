
SRCS=$(wildcard *.cpp)

OBJS=$(SRCS:.cpp=.o)

LINK_OBJS:= $(addprefix ../lib/,$(OBJS))

PROGS = $(patsubst %.cpp,../lib/%.o,$(SRCS))

CXX = g++
LIBNAME = VEther
LIBPATH = ../lib
CXXFLAGS = -fPIC -DPIC -I ../include -I ../ -I ../glfw/include $(SHARED_FLAGS)

all : $(PROGS) genlib
../lib/%.o: %.cpp
	$(CXX) -c $(CXXFLAGS) -o $@ $<	

clean:
	rm -f $(LINK_OBJS) $(LIBPATH)/lib$(LIBNAME).a

genlib:
	ar -cvr $(LIBPATH)/lib$(LIBNAME).a $(LINK_OBJS)
	gcc-ranlib $(LIBPATH)/lib$(LIBNAME).a