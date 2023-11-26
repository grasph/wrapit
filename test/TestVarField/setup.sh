if [ X`uname`  = XDarwin ]; then
  linker=DYLD
else
  linker=LD
fi

export JULIA_LOAD_PATH="@:@v#.#:@stdlib:`pwd`/TestVarFieldOn/src:`pwd`/TestVarFieldOff/src"
export ${linker}_LIBRARY_PATH="`pwd`/libTestVarFieldOn:`pwd`/libTestVarFieldOff"

