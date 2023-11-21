if [ X`uname`  = XDarwin ]; then
  linker=DYLD
else
  linker=LD
fi

export JULIA_LOAD_PATH="@:@v#.#:@stdlib:`pwd`/TestSizet/src"
export ${linker}_LIBRARY_PATH="`pwd`/libTestSizet"

