UNAME = $(shell uname)
JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(patsubst -std=gnu%,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)

CXXWRAP_PREFIX=$(shell julia -e 'import Pkg; haskey(Pkg.dependencies(), Base.UUID("1f15a43c-97ca-5a2a-ae31-89f07a497df4")) || Pkg.add("CxxWrap"); using CxxWrap; print(CxxWrap.prefix_path())')

CXXWRAP_CPPFLAGS=-I $(CXXWRAP_PREFIX)/include -I . --std=c++17
LDLIBS +=-L $(CXXWRAP_PREFIX)/lib -lcxxwrap_julia

ifeq ($(UNAME),Darwin)
  SO_SUFFIX = .dylib
else
  SO_SUFFIX = .so
endif

CXXFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -fmax-errors=3

CPPFLAGS += -MMD

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

CC_FILES=jlHello.cxx

.PHONY: all clean run_demo

.PRECIOUS: $(CC_FILES)

PRODUCTS=libHello/libjlHello$(SO_SUFFIX) $(addprefix libHello/src/, jlHello.cxx  jlHello.h  Wrapper.h generated_cxx) Hello/src/Hello.jl 

all: $(PRODUCTS)

clean:
	-$(RM) -r $(PRODUCTS) libHello/build jlHello-report.txt

libHello/src/jl%.cxx: %.wit
	wrapit --force $<

libHello/build/%.o: libHello/src/%.cxx
	-mkdir libHello/build
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

libHello/libjlHello$(SO_SUFFIX): libHello/build/jlHello.o
	$(LINK.o) -o $@ --shared -fPIC $^ $(ROOT_LIBS) $(LDLIBS)

echo_%:
	@echo $*=$($*)

run_demo: all
	. ./setup.sh && julia demo_Hello.jl

-include $(addsuffix .d, $(basename $(CC_FILES)))
