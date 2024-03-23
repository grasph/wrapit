UNAME = $(shell uname)

BUILD_DIR=build

JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(patsubst -std=gnu%,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)
WRAPIT = $(shell which wrapit)
WIT_FILE = Hello.wit
CXXWRAP_VERSION=$(shell julia -e "import TOML; print(get(TOML.parse(open(\"$(WIT_FILE)\")), \"cxxwrap_version\", \"\"));")

ifeq ($(CXXWRAP_VERSION),)
CXXWRAP_PREFIX=$(shell mkdir -p $(BUILD_DIR); julia --project=$(BUILD_DIR) -e "import Pkg; Pkg.add(\"CxxWrap\"); Pkg.resolve(); import CxxWrap; print(CxxWrap.prefix_path())")
CXXWRAP_VERSION:=$(shell julia --project=${BUILD_DIR} -e "import CxxWrap; print(pkgversion(CxxWrap));")
WRAPIT_OPT=--add-cfg cxxwrap_version=\"$(CXXWRAP_VERSION)\"
else
CXXWRAP_PREFIX=$(shell mkdir -p $(BUILD_DIR); julia --project=$(BUILD_DIR) -e "import Pkg; Pkg.add(name=\"CxxWrap\", version=\"$(CXXWRAP_VERSION)\"); Pkg.resolve(); import CxxWrap; print(CxxWrap.prefix_path())")
endif

CXXWRAP_CPPFLAGS=-I $(CXXWRAP_PREFIX)/include -I . --std=c++17
LDLIBS +=-L $(CXXWRAP_PREFIX)/lib -lcxxwrap_julia -lcxxwrap_julia_stl
CXXFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable

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

CC_FILES=jlHello.cxx

.PHONY: all clean run_demo

.PRECIOUS: $(CC_FILES)

PRODUCTS=$(addprefix $(BUILD_DIR)/, Hello/deps/libjlHello$(SO_SUFFIX) $(addprefix libHello/src/, jlHello.cxx  jlHello.h  Wrapper.h generated_cxx) Hello/src/Hello.jl jlHello-report.txt)

all: $(PRODUCTS)

clean:
	-$(RM) -r $(PRODUCTS)

$(BUILD_DIR)/libHello/src/jl%.cxx: %.wit
	$(WRAPIT) $(WRAPIT_OPT) --force --output-prefix $(BUILD_DIR) $<

$(BUILD_DIR)/%.o: $(BUILD_DIR)/libHello/src/%.cxx
	-mkdir -p $(BUILD_DIR)
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

$(BUILD_DIR)/Hello/deps/libjlHello$(SO_SUFFIX): $(BUILD_DIR)/jlHello.o
	-mkdir -p $(BUILD_DIR)/Hello/deps
	$(LINK.o) -o $@ --shared -fPIC $^ $(ROOT_LIBS) $(LDLIBS)

echo_%:
	@echo $*=$($*)

run_demo: all
	JULIA_LOAD_PATH=:$(BUILD_DIR) julia demo_Hello.jl

-include $(addprefix $(BUILD_DIR)/, $(addsuffix .d, $(basename $(CC_FILES))))
