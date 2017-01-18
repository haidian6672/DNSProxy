CLRD_ROOT = ./
OUT_DIR = $(CLRD_ROOT)/result

CLRD_DEFINES = -DTARGET=\"$(shell gcc -v 2> /dev/stdout | grep Target | cut -d ' ' -f 2)\" \
	       -I./inc \
	       -std=c++11 \
			 
THIRDPARTY = -I./3rd/jsoncpp -I./3rd/socklite -I./3rd/cpputility

ifeq "$(MAKECMDGOALS)" "release"
	DEFINES = $(CLRD_DEFINES) $(THIRDPARTY) -DCLEANDNS_RELEASE -DRELEASE
	CPPFLAGS = 
	CFLAGS = -O2 -Wall $(DEFINES) -fPIC -pthread
	CXXFLAGS = -O2 -Wall $(DEFINES) -fPIC -pthread
	CMDGOAL_DEF := $(MAKECMDGOALS)
else
	ifeq "$(MAKECMDGOALS)" "withpg"
		DEFINES = $(CLRD_DEFINES) $(THIRDPARTY) -DCLEANDNS_WITHPG -DWITHPG -DDEBUG
		CPPFLAGS = 
		CFLAGS = -g -pg -Wall $(DEFINES) -fPIC -pthread
		CXXFLAGS = -g -pg -Wall $(DEFINES) -fPIC -pthread
		CMDGOAL_DEF := $(MAKECMDGOALS)
	else
		DEFINES = $(CLRD_DEFINES) $(THIRDPARTY) -DCLEANDNS_DEBUG -DDEBUG
		CPPFLAGS =
		CFLAGS = -g -Wall $(DEFINES) -fPIC -pthread
		CXXFLAGS = -g -Wall $(DEFINES) -fPIC -pthread
		CMDGOAL_DEF := $(MAKECMDGOALS)
	endif
endif

CC	 = g++
CPP	 = g++
CXX	 = g++
AR	 = ar

CPP_FILES = $(wildcard ./src/*.cpp) 3rd/jsoncpp/jsoncpp.cpp 3rd/socklite/socketlite.cpp $(wildcard 3rd/cpputility/*.cpp)
OBJ_FILES = $(CPP_FILES:.cpp=.o)

STATIC_LIBS = 
DYNAMIC_LIBS = 
EXECUTABLE = dnsProxy

all	: PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake

PreProcess :
	@if test -d $(OUT_DIR); then rm -rf $(OUT_DIR); fi
	@mkdir -p $(OUT_DIR)/bin
	@mkdir -p $(OUT_DIR)/conf
	@mkdir -p $(OUT_DIR)/log
	@echo $(CPP_FILES)

cmdgoalError :
	@echo '***************************************************'
	@echo '******You must specified a make command goal.******'
	@echo '***************************************************'

debugclean :
	@rm -rf $(TEST_ROOT)/debug

relclean :
	@rm -rf $(TEST_ROOT)/release

pgclean : 
	@rm -rf $(TEST_ROOT)/withpg

clean :
	rm -vf src/*.o; rm -vf tools/*.o; rm -vf 3rd/jsoncpp/*.o; rm -vf 3rd/socklite/*.o; rm -vf 3rd/cpputility/*.o; rm -rf $(OUT_DIR)

install:
	@if [ -f ./result/bin/dnsProxy ]; then cp -v ./result/bin/dnsProxy /usr/bin/; else echo "please make first"; fi

AfterMake : 
	@if [ "$(MAKECMDGOALS)" == "release" ]; then rm -vf src/*.o; rm -vf 3rd/jsoncpp/*.o; rm -vf 3rd/socklite/*.o; rm -vf 3rd/cpputility/*.o; fi
	@mv -vf $(CLRD_ROOT)/dnsProxy $(OUT_DIR)/bin/dnsProxy

debug : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

release : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

withpg : PreProcess $(STATIC_LIBS) $(DYNAMIC_LIBS) $(EXECUTABLE) $(TEST_CASE) AfterMake
	@exit 0

%.o: src/%.cpp 
	$(CC) $(CXXFLAGS) -c -o $@ $<

dnsProxy: $(OBJ_FILES)
	$(CC) -o $@ $^ $(CXXFLAGS) -lresolv
