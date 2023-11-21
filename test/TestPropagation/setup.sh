if [ X`uname`  = XDarwin ]; then
  linker=DYLD
else
  linker=LD
fi

export JULIA_LOAD_PATH="@:@v#.#:@stdlib:`pwd`/TestPropagation1/src:`pwd`/TestPropagation2/src:`pwd`/TestPropagation3/src"
export ${linker}_LIBRARY_PATH="`pwd`/libTestPropagation1:`pwd`/libTestPropagation2:`pwd`/libTestPropagation3"

