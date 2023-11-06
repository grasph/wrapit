JL_SHARE = $(shell julia -e 'print(joinpath(Sys.BINDIR, Base.DATAROOTDIR, "julia"))')
CXXFLAGS += $(subst -std=gnu99,,$(shell $(JL_SHARE)/julia-config.jl --cflags))
#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation
LDFLAGS  += $(shell $(JL_SHARE)/julia-config.jl --ldflags)
LDLIBS   += $(shell $(JL_SHARE)/julia-config.jl --ldlibs)

CXXWRAP_CPPFLAGS=-I $(shell julia -e 'using CxxWrap; print(CxxWrap.prefix_path() * "/include")') -std=c++17

CXXFLAGS += -Wno-unused-variable -Wno-unused-but-set-variable -fmax-errors=3 -I.

CPPFLAGS += -MMD

LINK.o = $(CXX) $(LDFLAGS) $(TARGET_ARCH)

MODULE_NAMES=TestPropagation1 TestPropagation2 TestPropagation3

.PHONY: all clean test

CXX_PRODUCTS=$(foreach module,$(MODULE_NAMES), lib$(module)/src/jl$(module).cxx)
HEADERS=$(patsubst %.cxx, %.h, $(CXX_PRODUCTS))
OBJS=$(addsuffix .o, $(basename $(CXX_PRODUCTS)))
DEPS=$(addsuffix .d, $(basename $(CXX_PRODUCTS)))
JL_MODULES=$(foreach module,$(MODULE_NAMES), $(module)/src/$(module).jl)
JL_LIBS=$(foreach module,$(MODULE_NAMES), lib$(module)/libjl$(module).so)
BUILD_DIRS=$(foreach module,$(MODULE_NAMES), lib$(module)/build/)

PRODUCTS=$(JL_LIBS) $(CXX_PRODUCTS) $(HEADERS) $(JL_MODULES) 

.PRECIOUS: $(PRODUCTS)


all: $(PRODUCTS)

clean:
	-$(RM) $(PRODUCTS) -r $(BUILD_DIRS) $(patsubst %,jl%-report.txt,$(MODULE_NAMES)) $(OBJS) $(DEPS)
	[ -d libTestPropagation1/src ] && cd libTestPropagation1/src && $(RM) $(file < libTestPropagation1/src/generated_cxx) dbg_msg.h  generated_cxx  Wrapper.h || true
	-rmdir -p libTestPropagation1/src TestPropagation1/src 2>/dev/null || true
	[ -d libTestPropagation2/src ] && cd libTestPropagation2/src && $(RM) $(file < libTestPropagation2/src/generated_cxx) dbg_msg.h  generated_cxx  Wrapper.h || true
	-rmdir -p libTestPropagation2/src TestPropagation2/src 2>/dev/null || true
	[ -d libTestPropagation3/src ] && cd libTestPropagation3/src && $(RM) $(file < libTestPropagation3/src/generated_cxx) dbg_msg.h  generated_cxx  Wrapper.h || true
	-rmdir -p libTestPropagation3/src TestPropagation3/src 2>/dev/null || true


libTestPropagation1/src/jl%.cxx: %.wit
	wrapit --force $<

libTestPropagation2/src/jl%.cxx: %.wit
	wrapit --force $<

libTestPropagation3/src/jl%.cxx: %.wit
	wrapit --force $<

libTestPropagation1/build/%.o: libTestPropagation1/src/%.cxx
	[ -d libTestPropagation1/build ] || mkdir libTestPropagation1/build
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

libTestPropagation2/build/%.o: libTestPropagation2/src/%.cxx
	[ -d libTestPropagation2/build ] || mkdir libTestPropagation2/build
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

libTestPropagation3/build/%.o: libTestPropagation3/src/%.cxx
	[ -d libTestPropagation3/build ] || mkdir libTestPropagation3/build
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

libTestPropagation1/libjl%.so: libTestPropagation1/build/jl%.o
	$(LINK.o) -o $@ --shared -fPIC $<
	touch $*/src/$*.jl

libTestPropagation2/libjl%.so: libTestPropagation2/build/jl%.o
	$(LINK.o) -o $@ --shared -fPIC $<
	touch $*/src/$*.jl

libTestPropagation3/libjl%.so: libTestPropagation3/build/jl%.o
	$(LINK.o) -o $@ --shared -fPIC $<
	touch $*/src/$*.jl

echo_%:
	 @echo $*=$($*)

test:
	$(MAKE) all && . ./setup.sh && { julia runTestPropagation1.jl; julia runTestPropagation2.jl; julia runTestPropagation3.jl; }

-include $(DEPS)

