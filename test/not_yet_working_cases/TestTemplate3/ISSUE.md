## Issue with this test

With the generated wrapper, the Julia module load fails with error `No appropriate factory for type St3setINSt7__cxx1112basic_stringIcSt11char_traitsIcESaIcEEESt4lessIS5_ESaIS5_E`. It corresponds to the type, `std::set<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >, std::less<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > >, std::allocator<std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> > > >`

See `jlTestTemplate3.cxx.ref2` for a working wrapper. 

