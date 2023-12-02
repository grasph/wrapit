#CXXFLAGS += -DVERBOSE_IMPORT #To enable the verbose mode of the libray loading
#CXXFLAGS += -Wall -O0 -g     #To compile with debugger infomation

MODULE_NAME=TestStdVector
WRAPIT_PRODUCTS=JlA.cxx JlEltType1.cxx JlEltType2_.cxx JlEltType3_.cxx JlEltType4_.cxx JlEltType5_.cxx JlEltType6.cxx JlEltType7.cxx JlGlobals.cxx JlMyString.cxx

#EXTRA_OBJS=$(addprefix libTestStdVector/build/, JlA.o JlEltType1.o JlEltType2_.o JlEltType3_.o JlEltType4_.o JlEltType5_.o JlEltType6.o JlEltType7.o JlGlobals.o JlMyString.o)
EXTRA_OBJS+=libTestStdVector/build/A.o
#CPPFLAGS+=-DVERBOSE_IMPORT

-include ../make.rules

lib$(MODULE_NAME)/src/A.cxx: A.cxx
	cp -a $^ lib$(MODULE_NAME)/src/
