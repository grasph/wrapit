// Following method is defined twice, using respectively Long_t and Long64_t,
// which are the same on a 64 architecture, to name the 3rd argument type.
// Need to veto one of the two
int TSystem::GetPathInfo(const char *, Long_t *, Long64_t *, Long_t *, Long_t *);

// Protected access of TROOT::new prevents TROOT contructor wrapper to work,
void TROOT::TROOT(const char *, const char *, VoidFuncPtr_t *);

// Wrapper failes to compile due to the return type, void (*)()
// (complains a conversion method to a Julia type is missing).
Func_t TSystem::DynFindSymbol(const char *, const char *);

// Following method has a similar one, where parameters of the callback
// function are declared as const. Only one signature of callback function
// can be wrapped.
void TF1::TF1(const char *, Double_t (*)(Double_t *, Double_t *), Double_t, Double_t, Int_t, Int_t, TF1::EAddToList);
