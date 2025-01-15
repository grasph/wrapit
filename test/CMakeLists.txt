install(FILES WrapitTestSetup.cmake
              make.rules
              runtests.jl
              test_examples.jl
DESTINATION share/wrapit/test)

install(FILES TestAccessAndDelete/CMakeLists.txt
              TestAccessAndDelete/TestAccessAndDelete.h
              TestAccessAndDelete/TestAccessAndDelete.wit
              TestAccessAndDelete/compileandrun
              TestAccessAndDelete/runTestAccessAndDelete.jl
DESTINATION share/wrapit/test/TestAccessAndDelete)

install(FILES TestAutoAdd/.gitignore
              TestAutoAdd/A.h
              TestAutoAdd/B.h
              TestAutoAdd/CMakeLists.txt
              TestAutoAdd/Makefile
              TestAutoAdd/TestAutoAdd.wit
              TestAutoAdd/compileandrun
              TestAutoAdd/runTestAutoAdd.jl
              TestAutoAdd/setup.sh
DESTINATION share/wrapit/test/TestAutoAdd)

install(FILES TestCtorDefVal/A.h
              TestCtorDefVal/CMakeLists.txt
              TestCtorDefVal/TestCtorDefVal.wit
              TestCtorDefVal/compileandrun
              TestCtorDefVal/jlTestCtorDefVal.cxx.ref
              TestCtorDefVal/runTestCtorDefVal.jl
DESTINATION share/wrapit/test/TestCtorDefVal)

install(FILES TestEmptyClass/A.h
              TestEmptyClass/CMakeLists.txt
              TestEmptyClass/TestEmptyClass.wit
              TestEmptyClass/compileandrun
              TestEmptyClass/runTestEmptyClass.jl
              TestEmptyClass/vetoTestEmplyClass.txt
DESTINATION share/wrapit/test/TestEmptyClass)

install(FILES TestEnum/A.h
              TestEnum/CMakeLists.txt
              TestEnum/Makefile
              TestEnum/TestEnum.wit
              TestEnum/compileandrun
              TestEnum/runTestEnum.jl
DESTINATION share/wrapit/test/TestEnum)

install(FILES TestIncompleteArray/.gitignore
              TestIncompleteArray/A.h
              TestIncompleteArray/Makefile
              TestIncompleteArray/TestIncompleteArray.wit
              TestIncompleteArray/runTestIncompleteArray.jl
              TestIncompleteArray/setup.sh
DESTINATION share/wrapit/test/TestIncompleteArray)

install(FILES TestInheritance/A.h
              TestInheritance/C.h
              TestInheritance/CMakeLists.txt
              TestInheritance/D.h
              TestInheritance/Makefile
              TestInheritance/MyString.h
              TestInheritance/TestInheritance.wit
              TestInheritance/compileandrun
              TestInheritance/runTestInheritance.jl
              TestInheritance/setup.sh
DESTINATION share/wrapit/test/TestInheritance)

install(FILES TestNamespace/A.h
              TestNamespace/CMakeLists.txt
              TestNamespace/TestNamespace.wit
              TestNamespace/compileandrun
              TestNamespace/runTestNamespace.jl
DESTINATION share/wrapit/test/TestNamespace)

install(FILES TestNoFinalizer/A.cc
              TestNoFinalizer/A.h
              TestNoFinalizer/CMakeLists.txt
              TestNoFinalizer/TestNoFinalizer.wit
              TestNoFinalizer/compileandrun
              TestNoFinalizer/runTestNoFinalizer.jl
DESTINATION share/wrapit/test/TestNoFinalizer)

install(FILES TestOperators/A.h
              TestOperators/CMakeLists.txt
              TestOperators/TestOperators.wit
              TestOperators/compileandrun
              TestOperators/runTestOperators.jl
              TestOperators/setup.sh
DESTINATION share/wrapit/test/TestOperators)

install(FILES TestOrder/A.cxx
              TestOrder/A.h
              TestOrder/CMakeLists.txt
              TestOrder/TestOrder.wit
              TestOrder/compileandrun
              TestOrder/runTestOrder.jl
DESTINATION share/wrapit/test/TestOrder)

install(FILES TestPointers/A.h
              TestPointers/CMakeLists.txt
              TestPointers/TestPointers.wit
              TestPointers/compileandrun
              TestPointers/runTestPointers.jl
DESTINATION share/wrapit/test/TestPointers)

install(FILES TestPropagation/A.h
              TestPropagation/B1.h
              TestPropagation/B2.h
              TestPropagation/Makefile
              TestPropagation/README
              TestPropagation/TestPropagation1.wit
              TestPropagation/TestPropagation2.wit
              TestPropagation/TestPropagation3.wit
              TestPropagation/compileandtest
              TestPropagation/runTestPropagation.jl
              TestPropagation/runTestPropagation1.jl
              TestPropagation/runTestPropagation2.jl
              TestPropagation/runTestPropagation3.jl
DESTINATION share/wrapit/test/TestPropagation)

install(FILES TestSizet/A.h
              TestSizet/CMakeLists.txt
              TestSizet/TestSizet.wit
              TestSizet/compileandrun
              TestSizet/runTestSizet.jl
DESTINATION share/wrapit/test/TestSizet)

install(FILES TestStdString/CMakeLists.txt
              TestStdString/TestStdString.h
              TestStdString/TestStdString.wit
              TestStdString/compileandrun
              TestStdString/runTestStdString.jl
DESTINATION share/wrapit/test/TestStdString)

install(FILES TestStdVector/A.cxx
              TestStdVector/A.h
              TestStdVector/CMakeLists.txt
              TestStdVector/TestStdVector.wit
              TestStdVector/compileandrun
              TestStdVector/runTestStdVector.jl
DESTINATION share/wrapit/test/TestStdVector)

install(FILES TestStringView/A.h
              TestStringView/CMakeLists.txt
              TestStringView/TestStringView.wit
              TestStringView/compileandrun
              TestStringView/jlTestStringView.cxx.ref
              TestStringView/runTestStringView.jl
              TestStringView/setup.sh
DESTINATION share/wrapit/test/TestStringView)

install(FILES TestTemplate1/A.h
              TestTemplate1/CMakeLists.txt
              TestTemplate1/TestTemplate1.wit
              TestTemplate1/compileandrun
              TestTemplate1/runTestTemplate1.jl
DESTINATION share/wrapit/test/TestTemplate1)

install(FILES TestTemplate2/A.h
              TestTemplate2/CMakeLists.txt
              TestTemplate2/Makefile
              TestTemplate2/TestTemplate2.wit
              TestTemplate2/compileandrun
              TestTemplate2/jlTestTemplate2.jl
              TestTemplate2/runTestTemplate2.jl
DESTINATION share/wrapit/test/TestTemplate2)

install(FILES TestUsingType/A.h
              TestUsingType/CMakeLists.txt
              TestUsingType/TestUsingType.wit
              TestUsingType/compileandrun
              TestUsingType/runTestUsingType.jl
DESTINATION share/wrapit/test/TestUsingType)

install(FILES TestVarField/A.cxx
              TestVarField/A.h
              TestVarField/C.h
              TestVarField/Makefile
              TestVarField/Makefile.off
              TestVarField/Makefile.on
              TestVarField/TestVarFieldOff.wit
              TestVarField/TestVarFieldOn.wit
              TestVarField/compileandrun
              TestVarField/runTestVarField.jl
              TestVarField/runTestVarFieldOff.jl
              TestVarField/runTestVarFieldOn.jl
DESTINATION share/wrapit/test/TestVarField)

