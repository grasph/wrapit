#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation

MODULE_NAME = TestEmptyClass
CXX_PRODUCTS += $(addprefix lib$(MODULE_NAME)/src/, JlA1.cxx JlA2.cxx JlA3.cxx JlA4.cxx JlA5.cxx JlA6.cxx)

-include ../make.rules

$(CXX_PRODUCTS): lib$(MODULE_NAME)/src/jl$(MODULE_NAME).cxx

WRAPIT_VERBOSITY=1
