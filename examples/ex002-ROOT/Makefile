JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(subst -std=gnu99,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)

CXXWRAP_CPPFLAGS=-I $(shell julia -e 'using CxxWrap; print(CxxWrap.prefix_path() * "/include")') -std=c++17

CXXFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -I/home/pgras/git.d/root_build/include -fmax-errors=3

CPPFLAGS += -MMD

ROOT_LIBS = $(shell root-config --libs)
ROOT_INC_DIR = $(shell root-config --incdir)

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

CC_FILES=jlROOT.cxx

.PHONY: all clean run_demo check_root

.PRECIOUS: $(CC_FILES)

PRODUCTS=jlROOT.so jlROOT.cxx jlROOT.h ROOT-export.jl ROOT-generated.jl ROOT-generated.wit
all: $(PRODUCTS)

clean:
	-$(RM) $(PRODUCTS) jlROOT-report.txt jlROOT.o jlROOT.d

ROOT-generated.wit: ROOT.wit
	$(MAKE) check_root
	$(shell echo "#\n# Warning: file generated automatically from $<\n#" > $@)
	$(shell sed "s@%ROOT_INC_DIR%@$(ROOT_INC_DIR)@" $< >> $@ || rm $@)

check_root:
ifeq ($(ROOT_LIBS),)
	$(error ERROR: "Command root-config not found. ROOT (http:://root.cern.ch) environment needs to be set")
endif

run_demo: all
	  LD_LIBRARY_PATH=. JULIA_LOAD_PATH=.:@:@v#.#:@stdlib julia -i TDemo1.jl

jlROOT.cxx: ROOT-generated.wit jlROOT-veto.h Extra.h
	wrapit --force $<

%.o: %.cxx
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

jl%.so: jl%.o check_root
	$(LINK.o) -o $@ --shared -fPIC $< $(ROOT_LIBS)

echo_%:
	@echo "$* = $(subst ",\",$($*))"

-include $(addsuffix .d, $(basename $(CC_FILES)))
