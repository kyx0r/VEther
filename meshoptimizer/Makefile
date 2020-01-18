.SUFFIXES:
MAKEFLAGS+=-r

config=release
BUILD=../lib/

LIBRARY_SOURCES=$(wildcard src/*.cpp)
LIBRARY_OBJECTS=$(LIBRARY_SOURCES:%=$(BUILD)/%.o)

OBJECTS=$(LIBRARY_OBJECTS)
LIBRARY=$(BUILD)/libmeshoptimizer.a

CFLAGS=-g -Wall -Wextra -Werror -std=c89
CXXFLAGS=-g -Wall -Wextra -Wshadow -Wno-missing-field-initializers -Werror -std=c++98
LDFLAGS=

ifeq ($(config),iphone)
	IPHONESDK=/Applications/Xcode.app/Contents/Developer/Platforms/iPhoneOS.platform/Developer/SDKs/iPhoneOS.sdk
	CFLAGS+=-arch armv7 -arch arm64 -isysroot $(IPHONESDK)
	CXXFLAGS+=-arch armv7 -arch arm64 -isysroot $(IPHONESDK) -stdlib=libc++
	LDFLAGS+=-arch armv7 -arch arm64 -isysroot $(IPHONESDK) -L $(IPHONESDK)/usr/lib -mios-version-min=7.0
endif

ifeq ($(config),trace)
	CXXFLAGS+=-DTRACE=2
endif

ifeq ($(config),release)
	CXXFLAGS+=-O3 -DNDEBUG
endif

ifeq ($(config),coverage)
	CXXFLAGS+=-coverage
	LDFLAGS+=-coverage
endif

ifeq ($(config),sanitize)
	CXXFLAGS+=-fsanitize=address,undefined -fno-sanitize-recover=all
	LDFLAGS+=-fsanitize=address,undefined
endif

ifeq ($(config),analyze)
	CXXFLAGS+=--analyze
endif

all: $(LIBRARY)

$(LIBRARY): $(LIBRARY_OBJECTS)
	ar rcs $@ $^

$(BUILD)/%.cpp.o: %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $< $(CXXFLAGS) -c -MMD -MP -o $@

$(BUILD)/%.c.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $< $(CFLAGS) -c -MMD -MP -o $@

clean:
	rm -rf $(BUILD)/src $(LIBRARY) 

.PHONY: all clean format
