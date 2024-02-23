#ifndef TBRANCHPTR_H
#define TBRANCHPTR_H

#include "TBranch.h"

/**
 * Wrapper class for TBranch* that holds as template parameter the type of the
 * data stored in the branch leaf. It is used to replace the
 * TBranch::SetAdress(void*) method with a safer method that used a typed
 * pointer as argument instead of the void*.
 */
template<typename T>
class TBranchPtr{
  TBranch* p;
  
public:
  TBranchPtr(TBranch* p): p(p){};
  operator TBranch*() const {return p;}
  TBranch* operator->() const {return p;}
  TBranch& operator*() const {return *p;}
};

/** To be used in place of template<typename T> TBranch* TTree:Branch(const
 *  char* name, Char_t* obj, Int_t bufsize, Int_t splitlevel);
 */
template<typename T>
TBranchPtr<T> Branch(TTree& tree, const char* name, T* obj, Int_t bufsize = 3200, Int_t splitlevel = 99){
  return TBranchPtr<T>(tree.Branch(name, obj, bufsize, splitlevel));
}


/** To be used in place of TBranch::SetAddress.
 */
template<typename T>
void SetAddress(TBranchPtr<T> pobj, T* addr){
  pobj->SetAddress(addr);
}

/** To be used in place of template<class T > Int_t TTree::SetBranchAddress(const
 *  char * bname, T* add, nullptr )
 *  ptr must be a pointer to an allocated object.
 */
template<typename T>
Int_t SetBranchAddress(TTree& tree, const char* bname, T* ptr){
  return tree.SetBranchAddress(bname, ptr);
}


#endif //TBRANCHPTR_H not defined
