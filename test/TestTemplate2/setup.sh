if [ X`uname`  = XDarwin ]; then
  linker=DYLD
else
  linker=LD
fi

export JULIA_LOAD_PATH="@:@v#.#:@stdlib:`pwd`/TestTemplate2/src"
export ${linker}_LIBRARY_PATH="`pwd`/libTestTemplate2"

