
CXX = g++
LIBNAME = bullet3
LIBPATH = ../lib
CXXFLAGS = $(SHARED_FLAGS) -fPIC -I . -I.. 

#dirwildcard=$(foreach d,$(wildcard $(1:=/*)),$(call dirwildcard,$d,$2) $(filter $(subst *,%,$2),$d))
#BulletCollision=$(call dirwildcard,.,*.cpp)
BulletCollision=$(wildcard ./*.cpp)

OBJS=$(BulletCollision:.cpp=.o)

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
