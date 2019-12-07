CCACHE_EXISTS := $(shell ccache -V)
ifdef CCACHE_EXISTS
    CC := ccache $(CC)
    CXX := ccache g++-8
endif
DEBUG ?= 1
CXXFLAGS_BASE = -c -g -pthread -fPIC -std=c++17 -Wall -DJDE_EXPORT_MARKETS -DBOOST_SYSTEM_NO_DEPRECATED -Wno-unknown-pragmas
#-Wno-unknown-pragmas -Wno-ignored-attributes -Wno-int-in-bool-context

BIN_DIR_BASE:=../../bin/
OUT_DIR_BASE := .obj/
LIBS = 
#-lprotobuf

ifeq ($(DEBUG), 1)
	OUT_DIR=$(OUT_DIR_BASE)debug2
	CXXFLAGS= $(CXXFLAGS_BASE) -O0 -I$(OUT_DIR)
	BIN_DIR=$(BIN_DIR_BASE)debug2
else
	OUT_DIR=$(OUT_DIR_BASE)release2
	CXXFLAGS= $(CXXFLAGS_BASE) -march=native -DNDEBUG -O3 -I$(OUT_DIR)
	BIN_DIR=$(BIN_DIR_BASE)release2
endif

OUTPUT_FILE = libJde.XZ.so
OUTPUT = $(BIN_DIR)/$(OUTPUT_FILE)

all: $(OUTPUT)
	$(NOECHO) $(NOOP)

OBJECTS = $(OUT_DIR)/XZ.o

LDFLAGS = -pthread -shared -Wl,-z,origin -Wl,-rpath='$$ORIGIN'

$(OUTPUT): $(OUT_DIR)/stdafx.h.gch $(OBJECTS)
	$(CXX) -L$(BIN_DIR) $(LDFLAGS) $(OBJECTS) -o$(OUTPUT) $(LIBS)

$(OUT_DIR)/stdafx.h.gch: pc.h
	$(CXX) $(CXXFLAGS) pc.h -o$(OUT_DIR)/stdafx.h.gch -I/home/duffyj/code/libraries/spdlog/include -I$$BOOST_ROOT
	#-I/home/duffyj/code/libraries/eigen  -I/home/duffyj/code/libraries/tws-api/source/cppclient/client -I/home/duffyj/code/libraries/json/include

$(OUT_DIR)/%.o: ./%.cpp ./%.h pc.h
	$(CXX) $(CXXFLAGS) $(INCLUDE) ./$< -o$@ -c -I/home/duffyj/code/libraries/spdlog/include 
# -I/home/duffyj/code/libraries/tws-api/source/cppclient/client -I/home/duffyj/code/libraries/eigen -I/home/duffyj/code/libraries/json/include

clean:
	rm -rf -d $(OUT_DIR)/*.*
	rm -rf -d $(OUTPUT)
