BUILD_DIR=build

JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(patsubst -std=gnu%,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)
WRAPIT = $(shell which wrapit)
CXXWRAP_PREFIX=$(shell ../../buildtools/print-cxxwrap-path.jl)

CXXWRAP_CPPFLAGS=-I $(CXXWRAP_PREFIX)/include -I . --std=c++17
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)

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

CXXFLAGS += -O2

CPPFLAGS += -MMD -I $(ROOT_INC_DIR) -I.

ROOT_LIBS = $(shell root-config --libs)
ROOT_INC_DIR = $(shell root-config --incdir)

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

.PHONY: all clean run_demo run_demo2 check_root make_lib_from_objs

.PRECIOUS: $(GENERATED_CXX) $(BUILD_DIR)/ROOT-generated.wit

PRODUCTS=$(addprefix $(BUILD_DIR)/, ROOT/deps/libjlROOT$(SO_SUFFIX) ROOT-generated.wit ROOT/src/ROOT.jl ROOT/src/iROOT.jl ROOT/src/ROOTex.jl ROOT/Project.toml ROOT/src/ROOT-export.jl)

LOGS=$(BUILD_DIR)/jlROOT-report.txt
GENERATED_CXX:=$(file < $(BUILD_DIR)/libROOT/src/generated_cxx)
OBJS=$(addprefix $(BUILD_DIR)/libROOT/build/, $(patsubst %.cxx,%.o, $(GENERATED_CXX)))
DEPS=$(patsubst %.o,%.d, $(OBJS))

all: $(PRODUCTS) 

clean:
	-$(RM) $(PRODUCTS) $(LOGS)  $(addprefix $(BUILD_DIR)/libROOT/src/, $(GENERATED_CXX) dbg_msg.h  generated_cxx  jlROOT.h  Wrapper.h $(DEPS) ROOT/src/ROOT-generated.jl)  ROOT-generated.wit $(OBJS)
	-rmdir $(BUILD_DIR)/libROOT/src $(BUILD_DIR)/libROOT/build $(BUILD_DIR)/libROOT $(BUILD_DIR)/ROOT/src $(BUILD_DIR)/ROOT/deps $(BUILD_DIR)/ROOT


$(BUILD_DIR)/ROOT-generated.wit: ROOT.wit
	[ -d $(BUILD_DIR) ] || mkdir $(BUILD_DIR)
	$(MAKE) check_root
	$(shell echo "#\n# Warning: file generated automatically from $<\n#" > $@)
	$(shell sed "s@%ROOT_INC_DIR%@$(ROOT_INC_DIR)@" $< >> $@ || rm $@)

check_root:
ifeq ($(ROOT_LIBS),)
	$(error ERROR: "Command root-config not found. ROOT (http:://root.cern.ch) environment needs to be set")
endif

run_demo: all
	LD_LIBRARY_PATH=libROOT:$(shell root-config --libdir) JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia -i demo_ROOT.jl

run_demo2: all
	LD_LIBRARY_PATH=libROOT:$(shell root-config --libdir) JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia -i demo_TGraph.jl

test: all
	LD_LIBRARY_PATH=libROOT:$(shell root-config --libdir) JULIA_LOAD_PATH=`pwd`/ROOT/src:@:@v#.#:@stdlib julia demo_ROOT.jl
	cmp demo_ROOT.png demo_ROOT-ref.png
	LD_LIBRARY_PATH=libROOT:$(shell root-config --libdir) JULIA_LOAD_PATH=`pwd`/ROOT/src:@:@v#.#:@stdlib julia demo_TGraph.jl
	cmp demo_TGraph.png demo_TGraph-ref.png

%/libROOT/src/generated_cxx %/ROOT/Project.toml %/ROOT/src/ROOT-export.jl: %/ROOT-generated.wit jlROOT-veto.h $(WRAPIT)
	$(MAKE) $*/ROOT-generated.wit
	$(WRAPIT) --force --update --output-prefix $* $<
	$(eval GENERATED_CXX:=$(file < $(BUILD_DIR)/libROOT/src/generated_cxx))
	$(eval OBJS:=$(addprefix $(BUILD_DIR)/libROOT/build/, $(patsubst %.cxx,%.o, $(GENERATED_CXX))))

$(BUILD_DIR)/libROOT/src/Templates.cxx: Templates.cxx
	[ -d $(BUILD_DIR)/libROOT/src ] || mkdir -p $(BUILD_DIR)/libROOT/src
	cp -a Templates.cxx $@

$(BUILD_DIR)/libROOT/build/%.o: $(BUILD_DIR)/libROOT/src/%.cxx
	[ -d $(BUILD_DIR)/libROOT/build ] || mkdir -p $(BUILD_DIR)/libROOT/build
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

$(BUILD_DIR)/ROOT/deps/libjlROOT$(SO_SUFFIX): $(BUILD_DIR)/libROOT/src/generated_cxx
	$(MAKE) check_root
	$(MAKE) $<
	$(MAKE) make_lib_from_objs

$(BUILD_DIR)/ROOT/src/%.jl: %.jl
	[ -d $(BUILD_DIR)/ROOT/src ] || mkdir -p $(BUILD_DIR)/ROOT/src
	cp -a $^ $(BUILD_DIR)/ROOT/src/


make_lib_from_objs: $(OBJS)
	[ -d $(BUILD_DIR)/ROOT/deps ] || mkdir -p $(BUILD_DIR)/ROOT/deps
	$(LINK.o) -o $(BUILD_DIR)/ROOT/deps/libjlROOT$(SO_SUFFIX) --shared -fPIC $(OBJS) $(ROOT_LIBS)

echo_%:
	@echo "$* = $(subst ",\",$($*))"

-include $(DEPS)
