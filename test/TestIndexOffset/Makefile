BUILD_DIR=build

UNAME = $(shell uname)

JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(patsubst -std=gnu%,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)


CXXWRAP_PREFIX:=$(shell julia --project=${BUILD_DIR} -e 'import Pkg; haskey(Pkg.dependencies(), Base.UUID("1f15a43c-97ca-5a2a-ae31-89f07a497df4")) || Pkg.add("CxxWrap"); using CxxWrap; print(CxxWrap.prefix_path())')
CXXWRAP_VERSION:=$(shell julia --project=${BUILD_DIR} -e "import CxxWrap; print(pkgversion(CxxWrap));")
WRAPIT_OPT=--add-cfg cxxwrap_version=\"$(CXXWRAP_VERSION)\"


CXXWRAP_CPPFLAGS=-I $(CXXWRAP_PREFIX)/include -std=c++20
LDLIBS +=-L $(CXXWRAP_PREFIX)/lib -lcxxwrap_julia -lcxxwrap_julia_stl
CXXFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -I.


CXX_IS_CLANG=$(shell $(CXX) -dM -E - < /dev/null | grep -c __clang__)

ifeq ($(UNAME),Darwin)
  SO_SUFFIX = .dylib
else
  SO_SUFFIX = .so
endif

ifneq ($(CXX_IS_CLANG), 0)
	CXXFLAGS += -ferror-limit=3
else #assuming gcc
	CXXFLAGS += -fmax-errors=3
endif


CPPFLAGS += -MMD

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

MODULE_NAMES=TestIndexOffset1 TestIndexOffset2

.PHONY: all clean test

CXX_PRODUCTS=$(foreach module,$(MODULE_NAMES), $(BUILD_DIR)/lib$(module)/src/jl$(module).cxx)
OBJS=$(foreach module,$(MODULE_NAMES), $(BUILD_DIR)/lib$(module)/build/jl$(module).o)
HEADERS=$(patsubst %.cxx, %.h, $(CXX_PRODUCTS))
DEPS=$(addsuffix .d, $(basename $(OBJS)))
JL_MODULES=$(foreach module,$(MODULE_NAMES), $(BUILD_DIR)/$(module)/src/$(module).jl)
JL_LIBS=$(foreach module,$(MODULE_NAMES), $(BUILD_DIR)/$(module)/deps/libjl$(module)$(SO_SUFFIX))

PRODUCTS=$(JL_LIBS) $(CXX_PRODUCTS) $(HEADERS) $(JL_MODULES) 

.PRECIOUS: $(PRODUCTS)

all: $(PRODUCTS)

clean:
	-$(RM) -r $(BUILD_DIR)

#use of % instead of $(BUILD_DIR) prevents make to run the recipee multiple times when running with -j
%/libTestIndexOffset1/src/jlTestIndexOffset1.cxx %/libTestIndexOffset1/src/jlTestIndexOffset1.h %/TestIndexOffset1/src/TestIndexOffset1.jl: TestIndexOffset1.wit
	wrapit $(WRAPIT_OPT) --force --output-prefix $* $<

%/libTestIndexOffset2/src/jlTestIndexOffset2.cxx %/libTestIndexOffset2/src/jlTestIndexOffset2.h %/TestIndexOffset2/src/TestIndexOffset2.jl: TestIndexOffset2.wit
	wrapit $(WRAPIT_OPT) --force --output-prefix $* $<

$(BUILD_DIR)/libTestIndexOffset1/build/%.o: $(BUILD_DIR)/libTestIndexOffset1/src/%.cxx
	[ -d $(@D) ] || mkdir -p $(@D)
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

$(BUILD_DIR)/libTestIndexOffset2/build/%.o: $(BUILD_DIR)/libTestIndexOffset2/src/%.cxx
	[ -d $(@D) ] || mkdir -p $(@D)
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $< $(LDLIBS)


$(BUILD_DIR)/TestIndexOffset1/deps/libjl%$(SO_SUFFIX): $(BUILD_DIR)/libTestIndexOffset1/build/jl%.o
	[ -d $(@D) ] || mkdir -p $(@D)
	$(LINK.o) -o $@ --shared -fPIC $< $(LDLIBS) 
	touch $(BUILD_DIR)/$*/src/$*.jl

$(BUILD_DIR)/TestIndexOffset2/deps/libjl%$(SO_SUFFIX): $(BUILD_DIR)/libTestIndexOffset2/build/jl%.o
	[ -d $(@D) ] || mkdir -p $(@D)
	$(LINK.o) -o $@ --shared -fPIC $< $(LDLIBS)
	touch $(BUILD_DIR)/$*/src/$*.jl


echo_%:
	 @echo $*=$($*)

test: all
	julia runTestIndexOffset.jl

-include $(DEPS)

