JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(subst -std=gnu99,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)

CXXWRAP_CPPFLAGS=-I $(shell julia -e 'using CxxWrap; print(CxxWrap.prefix_path() * "/include")') -std=c++17 -I.

CXXFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -fmax-errors=3

CPPFLAGS += -MMD

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

CC_FILES=jlHello.cxx

.PHONY: all clean run_demo

.PRECIOUS: $(CC_FILES)

PRODUCTS=libHello/libjlHello.so $(addprefix libHello/src/, jlHello.cxx  jlHello.h  Wrapper.h generated_cxx) Hello/src/Hello.jl 

all: $(PRODUCTS)

clean:
	-$(RM) $(PRODUCTS) -r libHello/build jlHello-report.txt

libHello/src/jl%.cxx: %.wit
	wrapit --force $<

libHello/build/%.o: libHello/src/%.cxx
	-mkdir libHello/build
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

libHello/libjlHello.so: libHello/build/jlHello.o
	$(LINK.o) -o $@ --shared -fPIC $^ $(ROOT_LIBS)

echo_%:
	@echo $*=$($*)

run_demo: all
	. ./setup.sh && julia demo_Hello.jl

-include $(addsuffix .d, $(basename $(CC_FILES)))
