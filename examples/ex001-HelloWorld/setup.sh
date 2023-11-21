export JULIA_LOAD_PATH="@:@v#.#:@stdlib:`pwd`/Hello/src"

if [ X`uname`  = XDarwin ]; then
  export DYLD_LIBRARY_PATH="`pwd`/libHello${DYLD_LIBRARY_PATH:+:$DYLD_LIBRARY_PATH}"
else
  export LD_LIBRARY_PATH="`pwd`/libHello${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
fi

