JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(subst -std=gnu99,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)

CXXWRAP_CPPFLAGS=-I $(shell julia -e 'using CxxWrap; print(CxxWrap.prefix_path() * "/include")') -std=c++17

CXXFLAGS += -fmax-errors=3 -O2

CPPFLAGS += -MMD -I $(ROOT_INC_DIR)

ROOT_LIBS = $(shell root-config --libs)
ROOT_INC_DIR = $(shell root-config --incdir)

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

.PHONY: all clean run_demo check_root make_lib_from_objs

.PRECIOUS: $(GENERATED_CXX) ROOT-generated.wit

PRODUCTS=libROOT/libjlROOT.so
GENERATED_CXX:=$(file < libROOT/src/generated_cxx)
OBJS=$(addprefix libROOT/build/, $(patsubst %.cxx,%.o, $(GENERATED_CXX)))

all: $(PRODUCTS)

clean:
	-$(RM) $(PRODUCTS) jlROOT-report.txt jlROOT.o jlROOT.d $(GENERATED_CXX) libROOT/src/generated_cxx

ROOT-generated.wit: ROOT.wit
	$(MAKE) check_root
	$(shell echo "#\n# Warning: file generated automatically from $<\n#" > $@)
	$(shell sed "s@%ROOT_INC_DIR%@$(ROOT_INC_DIR)@" $< >> $@ || rm $@)

check_root:
ifeq ($(ROOT_LIBS),)
	$(error ERROR: "Command root-config not found. ROOT (http:://root.cern.ch) environment needs to be set")
endif

run_demo: all
	LD_LIBRARY_PATH=.:$(shell root-config --libdir) JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia -i demo_ROOT.jl

run_demo2: all
	LD_LIBRARY_PATH=.:$(shell root-config --libdir) JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia -i demo_TGraph.jl

test: all
	LD_LIBRARY_PATH=libROOT:$(shell root-config --libdir) JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia demo_ROOT.jl
	cmp demo_ROOT.png demo_ROOT-ref.png
	LD_LIBRARY_PATH=libROOT:$(shell root-config --libdir) JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia demo_TGraph.jl
	cmp demo_TGraph.png demo_TGraph-ref.png

libROOT/src/generated_cxx: ROOT-generated.wit jlROOT-veto.h
	wrapit --force $<
	$(eval GENERATED_CXX:=$(file < libROOT/src/generated_cxx))
	$(eval OBJS:=$(addprefix libROOT/build/, $(patsubst %.cxx,%.o, $(GENERATED_CXX))))

libROOT/build/%.o: libROOT/src/%.cxx
	-mkdir -p libROOT/build
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

libROOT/libjlROOT.so: libROOT/src/generated_cxx
	$(MAKE) check_root
	$(MAKE) make_lib_from_objs

make_lib_from_objs: $(OBJS)
	$(LINK.o) -o libROOT/libjlROOT.so --shared -fPIC $(OBJS) $(ROOT_LIBS)

echo_%:
	@echo "$* = $(subst ",\",$($*))"

-include $(addprefix libROOT/build/, $(addsuffix .d, $(basename $(GENERATED_FILES))))

