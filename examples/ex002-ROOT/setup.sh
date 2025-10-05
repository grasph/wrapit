export JULIA_LOAD_PATH="`pwd`/build/ROOT/src:@:@v#.#:@stdlib:`pwd`/build/ROOT"
export LD_LIBRARY_PATH="`pwd`/libROOT${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export JULIA_PROJECT="`pwd`/build"
