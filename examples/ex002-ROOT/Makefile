JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(patsubst -std=gnu%,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)
WRAPIT = $(shell which wrapit)
CXXWRAP_CPPFLAGS=-I $(shell julia -e 'import Pkg; haskey(Pkg.dependencies(), Base.UUID("1f15a43c-97ca-5a2a-ae31-89f07a497df4")) || Pkg.add("CxxWrap"); using CxxWrap; print(CxxWrap.prefix_path() * "/include")') -std=c++17 -I.
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

.PRECIOUS: $(GENERATED_CXX) ROOT-generated.wit

PRODUCTS=libROOT/libjlROOT$(SO_SUFFIX)
GENERATED_CXX:=$(file < libROOT/src/generated_cxx)
OBJS=$(addprefix libROOT/build/, $(patsubst %.cxx,%.o, $(GENERATED_CXX)))
DEPS=$(patsubst %.o,%.d, $(OBJS))

all: $(PRODUCTS) ROOT/src/ROOT.jl ROOT/src/iROOT.jl ROOT/src/ROOTex.jl

clean:
	-$(RM) $(PRODUCTS) jlROOT-report.txt jlROOT.o jlROOT.d $(GENERATED_CXX) ROOT-generated.jl ROOT-generated.wit $(OBJS) \
libROOT/src/generated_cxx

ROOT-generated.wit: ROOT.wit
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
	LD_LIBRARY_PATH=libROOT:$(shell root-config --libdir) JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia demo_ROOT.jl
	cmp demo_ROOT.png demo_ROOT-ref.png
	LD_LIBRARY_PATH=libROOT:$(shell root-config --libdir) JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia demo_TGraph.jl
	cmp demo_TGraph.png demo_TGraph-ref.png

libROOT/src/generated_cxx: ROOT-generated.wit jlROOT-veto.h $(WRAPIT)
	$(WRAPIT) --force --update $<
	$(eval GENERATED_CXX:=$(file < libROOT/src/generated_cxx))
	$(eval OBJS:=$(addprefix libROOT/build/, $(patsubst %.cxx,%.o, $(GENERATED_CXX))))

libROOT/src/Templates.cxx: Templates.cxx
	cp -a Templates.cxx $@

libROOT/build/%.o: libROOT/src/%.cxx
	-mkdir -p libROOT/build
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

libROOT/libjlROOT$(SO_SUFFIX): libROOT/src/generated_cxx
	$(MAKE) check_root
	$(MAKE) make_lib_from_objs

ROOT/src/%.jl: %.jl
	cp -a $^ ROOT/src/

make_lib_from_objs: $(OBJS)
	$(LINK.o) -o libROOT/libjlROOT$(SO_SUFFIX) --shared -fPIC $(OBJS) $(ROOT_LIBS)

echo_%:
	@echo "$* = $(subst ",\",$($*))"

-include $(DEPS)

