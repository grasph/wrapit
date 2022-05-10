JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(subst -std=gnu99,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)

CXXWRAP_CPPFLAGS=-I $(shell julia -e 'using CxxWrap; print(CxxWrap.prefix_path() * "/include")') -std=c++17

CXXFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -fmax-errors=3

CPPFLAGS += -MMD

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

MODULE_NAME=TestPropagation

.PHONY: all clean
.PRECIOUS: jlTestPropagation1.cxx jlTestPropagation2.cxx jlTestPropagation3.cxx

PRODUCTS=jlTestPropagation1.so jlTestPropagation2.so jlTestPropagation3.so jlTestPropagation1.cxx jlTestPropagation2.cxx jlTestPropagation3.cxx jlTestPropagation1.h jlTestPropagation2.h jlTestPropagation3.h  TestPropagation1.jl  TestPropagation2.jl  TestPropagation3.jl


all: $(PRODUCTS)

clean:
	-$(RM) $(PRODUCTS) jlTestPropagation1.d jlTestPropagation2.d jlTestPropagation3.d

jl%.cxx: %.wit
	wrapit --force $<

%.o: %.cxx
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

jl%.so: jl%.o
	$(LINK.o) -o $@ --shared -fPIC $<

echo_%:
	 @echo $*=$($*)

test:
	$(MAKE) all && . ./setup.sh && { julia runTestPropagation1.jl; julia runTestPropagation2.jl; julia runTestPropagation3.jl; }


-include $(addsuffix .d, $(basename $(CC_FILES)))
