using Test

push!(LOAD_PATH, "$(@__DIR__)/build")
using TestPointers

using CxxWrap: CxxPtr, ConstCxxPtr

function runtest()
    @testset "Pointers test" begin
        #A::n static field counts the number of A instances:
        n_instances = A!n()
        #Following statement will create a new instance of A
        val = f_val()
        #Currently one extra copy is done when passing through the C++-Julia binding:
        n_instances += 2
        @test A!n() == n_instances
        
        #Following statements should return a ref or a pointer
        #with no new instance creation
        ref = f_ref()
        ptr = f_ptr()
        constref = f_constref()
        constptr = f_constptr()
        #check that no new instance was created:
        @test A!n() == n_instances
        uniqueptr = f_uniqueptr()
        sharedptr = f_sharedptr()
        weakptr = f_weakptr(sharedptr)
        
        @test i(ref) == i(ptr) == i(constref) == i(constptr)
        instance1_id = i(ref)
        instance2_id = i(val)
        instance3_id = i(uniqueptr[])
        instance4_id = i(sharedptr[])
    
        # Checking recipes provided by values_refs_and_ptrs.md
        #
        #   Check recipes for f(A):
        @test getiofacopy_val(val) > instance2_id #the copy increments i
        @test getiofacopy_val(constref) > instance1_id
        @test getiofacopy_val(ref) > instance1_id
        @test getiofacopy_val(constptr[]) >  instance1_id
        @test getiofacopy_val(ptr[]) > instance1_id
        @test getiofacopy_val(uniqueptr[]) > instance3_id
        @test getiofacopy_val(sharedptr[]) > instance4_id
        @test getiofacopy_val(weakptr[]) > instance4_id
        #   Check the direct call does not work and above f(A) recipes are needed:
        @test_throws Exception getiofacopy_val(ptr)
        @test_throws Exception getiofacopy_val(sharedptr)
        #
        #   Check that val was not modified:
        @test i(val) == instance2_id
        #
        #   Check recipes for f(const A& ):
        @test geti_constref(val) == instance2_id
        @test geti_constref(constref) == instance1_id
        @test geti_constref(ref) == instance1_id
        @test geti_constref(constptr[]) == instance1_id
        @test geti_constref(ptr[]) == instance1_id
        @test geti_constref(uniqueptr[]) == instance3_id
        @test geti_constref(sharedptr[]) == instance4_id
        @test geti_constref(weakptr[]) == instance4_id
        #   Check above f(A&) recipes are needed:
        @test_throws Exception geti_constref(constptr)
        @test_throws Exception geti_constref(ptr)
        @test_throws Exception geti_constref(sharedptr)
        #
        #   Check recipes for f(A& ):
        instance2_id = i(val) + 1
        @test inci_ref(val) == instance2_id
        @test i(val) == instance2_id
        #
        instance1_id = i(ref) + 1
        @test inci_ref(constref[]) == instance1_id
        @test i(ref) == instance1_id
        #
        instance1_id = i(ref) + 1
        @test inci_ref(ref) == instance1_id
        @test i(ref) == instance1_id
        #
        instance1_id = i(ref) + 1
        @test inci_ref(constptr[]) == instance1_id
        @test i(ref) == instance1_id
        #
        instance1_id = i(ref) + 1
        @test inci_ref(ptr[]) == instance1_id
        @test i(ref) == instance1_id
        #
        instance3_id = i(uniqueptr[]) + 1
        @test inci_ref(uniqueptr[]) == instance3_id
        @test i(uniqueptr[]) == instance3_id
        #
        instance4_id = i(sharedptr[]) + 1
        @test inci_ref(sharedptr[]) == instance4_id
        @test i(sharedptr[]) == instance4_id
        #    
        instance4_id = i(weakptr[]) + 1
        @test inci_ref(weakptr[]) == instance4_id
        @test i(weakptr[]) == instance4_id
        #
        #   Check above recipes for f(A&) are needed:
        @test_throws Exception inci_ref(constref)
        @test_throws Exception inci_ref(ptr)
        @test_throws Exception inci_ref(constptr)
        @test_throws Exception inci_ref(sharedptr)
        #
        #   Check recipes for f(const A*):
        @test geti_constptr(ConstCxxPtr(val)) == instance2_id
        @test geti_constptr(CxxPtr(val)) == instance2_id
        @test geti_constptr(ConstCxxPtr(ref)) == instance1_id
        @test geti_constptr(CxxPtr(ref)) == instance1_id
        @test geti_constptr(ConstCxxPtr(constref)) == instance1_id
        @test geti_constptr(CxxPtr(constref)) == instance1_id
        @test geti_constptr(ptr) == instance1_id
        @test geti_constptr(constptr) == instance1_id
        @test geti_constptr(ConstCxxPtr(uniqueptr[])) == instance3_id
        @test geti_constptr(CxxPtr(uniqueptr[])) == instance3_id
        @test geti_constptr(ConstCxxPtr(sharedptr[])) == instance4_id
        @test geti_constptr(CxxPtr(sharedptr[])) == instance4_id
        @test geti_constptr(ConstCxxPtr(weakptr[])) == instance4_id
        @test geti_constptr(CxxPtr(weakptr[])) == instance4_id
        #   Check above recipes for f(const A*) are needed:
        @test_throws Exception geti_constptr(val)
        @test_throws Exception geti_constptr(ref)
        @test_throws Exception geti_constptr(constref)
        @test_throws Exception geti_constptr(sharedptr)
        @test_throws Exception geti_constptr(sharedptr[])
        @test_throws Exception geti_constptr(CxxPtr(sharedptr))
        #
        #   Check recipes for f(A*):
        instance2_id = i(val) + 1
        @test inci_ptr(CxxPtr(val)) == instance2_id
        @test i(val) == instance2_id
        #
        instance1_id = i(ref) + 1
        @test inci_ptr(CxxPtr(constref)) == instance1_id
        @test i(ref) == instance1_id
        #
        instance1_id = i(ref) + 1
        @test inci_ptr(CxxPtr(ref)) == instance1_id
        @test i(ref) == instance1_id
        #
        instance1_id = i(ref) + 1
        @test inci_ptr(CxxPtr(constptr)) == instance1_id
        @test i(ref) == instance1_id
        #
        instance1_id = i(ref) + 1
        @test inci_ptr(ptr) == instance1_id
        @test i(ref) == instance1_id
        #
        instance3_id = i(uniqueptr[]) + 1
        @test inci_ptr(CxxPtr(uniqueptr[])) == instance3_id
        @test i(uniqueptr[]) == instance3_id
        #
        instance4_id = i(sharedptr[]) + 1
        @test inci_ptr(CxxPtr(sharedptr[])) == instance4_id
        @test i(sharedptr[]) == instance4_id
        #
        instance4_id = i(weakptr[]) + 1
        @test inci_ptr(CxxPtr(weakptr[])) == instance4_id
        @test i(weakptr[]) == instance4_id
        #
        #   Check above recipes for f(A*) are needed:
        @test_throws Exception inci_ptr(val)
        @test_throws Exception inci_ptr(ConstCxxPtr(val))
        @test_throws Exception inci_ptr(ref)
        @test_throws Exception inci_ptr(constref)
        @test_throws Exception inci_ptr(constptr)
        @test_throws Exception inci_ptr(constptr[])
        #   
        #   Check recipes for f(std::uniq_ptr<A>& a):
        instance3_id = i(uniqueptr[]) + 1
        @test inci_uniqueptr(uniqueptr) == instance3_id
        @test i(uniqueptr[]) == instance3_id
        #   Check recipes for f(std::shared_ptr<A> a):
        instance4_id = i(sharedptr[]) + 1
        @test inci_sharedptr(sharedptr) == instance4_id
        @test i(sharedptr[]) == instance4_id
        #   Check recipes for f(std::weak_ptr<A> a):
        instance4_id = i(weakptr[]) + 1
        @test inci_weakptr(weakptr) == instance4_id
        @test i(weakptr[]) == instance4_id
        # End of the recipe check
    end
end

if "-s" in ARGS #Serialize mode
    Test.TESTSET_PRINT_ENABLE[] = false
    serialize(stdout, runtest())
else
    runtest()
end
