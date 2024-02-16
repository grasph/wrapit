.PHONY: all clean test

all:
	$(MAKE) -f Makefile.on $@
	$(MAKE) -f Makefile.off $@

clean:
	$(MAKE) -f Makefile.on $@
	$(MAKE) -f Makefile.off $@

test: all
	julia runTestVarField.jl
