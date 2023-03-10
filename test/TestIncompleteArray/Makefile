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

MODULE_NAME=TestIncompleteArray

.PHONY: all clean test

.PRECIOUS: $(PRODUCT)

CC_PRODUCTS=jl$(MODULE_NAME).cxx
PRODUCTS=jl$(MODULE_NAME).so $(CC_PRODUCTS) jl$(MODULE_NAME).h $(MODULE_NAME).jl

all: $(PRODUCTS)

clean:
	-$(RM) $(PRODUCTS) jl$(MODULE_NAME).d jl$(MODULE_NAME).o jl$(MODULE_NAME)-report.txt

jl%.cxx: %.wit
	wrapit --force $<

%.o: %.cxx
	$(COMPILE.cc) $(CXXWRAP_CPPFLAGS) -o $@ $<

jl%.so: jl%.o
	$(LINK.o) -o $@ --shared -fPIC $<
	touch $(MODULE_NAME).jl

echo_%:
	 @echo $*=$($*)

test:
	$(MAKE) all && . ./setup.sh && julia run$(MODULE_NAME).jl


-include $(addsuffix .d, $(basename $(CC_PRODUCTS)))
