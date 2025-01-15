install(FILES ex001-HelloWorld/A.h
              ex001-HelloWorld/CMakeLists.txt
              ex001-HelloWorld/Hello.wit
              ex001-HelloWorld/Makefile
              ex001-HelloWorld/README.md
              ex001-HelloWorld/demo_Hello.jl
DESTINATION share/wrapit/examples/ex001-HelloWorld)

install(PROGRAMS ex001-HelloWorld/install.jl
DESTINATION share/wrapit/examples/ex001-HelloWorld)

install(FILES ex002-ROOT/Makefile
              ex002-ROOT/README.md
              ex002-ROOT/ROOT.jl
              ex002-ROOT/ROOT.wit
              ex002-ROOT/ROOTex.jl
              ex002-ROOT/TBranchPtr.h
              ex002-ROOT/TTree_examples/README.md
              ex002-ROOT/TTree_examples/read_tree1.jl
              ex002-ROOT/TTree_examples/read_tree2.jl
              ex002-ROOT/TTree_examples/read_tree3.jl
              ex002-ROOT/TTree_examples/write_tree1.jl
              ex002-ROOT/TTree_examples/write_tree2.jl
              ex002-ROOT/TTree_examples/write_tree3.jl
              ex002-ROOT/Templates.h
              ex002-ROOT/demo_ROOT-ref.png
              ex002-ROOT/demo_ROOT.jl
              ex002-ROOT/demo_TGraph-ref.png
              ex002-ROOT/demo_TGraph.jl
              ex002-ROOT/demo_fit_with_jl_func.jl
              ex002-ROOT/demo_fit_with_jl_func_2.jl
              ex002-ROOT/iROOT.jl
              ex002-ROOT/jlROOT-veto.h
              ex002-ROOT/setup.csh
              ex002-ROOT/setup.sh
DESTINATION share/wrapit/examples/ex002-ROOT)

install(PROGRAMS ex002-ROOT/install.jl
DESTINATION share/wrapit/examples/ex002-ROOT)


