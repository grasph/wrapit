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

/.*std::[io]stream.*/;

TTreeReader::Iterator_t TTreeReader::Iterator_t::operator++(int);
libROOT/src/JlClasses_008.cxx:379:    t.method("++", static_cast<TTreeReader::Iterator_t (TTreeReader::Iterator_t::*)(int) >(&TTreeReader::Iterator_t::operator++));
TTreeReader::Iterator_t & TTreeReader::Iterator_t::operator++();
void ROOT::RDataFrame::RDataFrame(std::unique_ptr<ROOT::RDF::RDataSource>, const ROOT::RDataFrame::ColumnNames_t &);
/ROOT::RDataFrame ROOT::Internal::.*/;

TLeaf::GetValueHelper;
// 
//Quad precision float not supported:
/.*LongDouble_t.*/;

/void TROOT::RegisterModule(.*)/;

//Duplicate uses with TBranch * TTree::Branch(const char *, void*, const char *, Int_t, Int_t);
//and the one with 2nd arg as int is giving a warning about duplicate methods.
TBranch * TTree::Branch(const char *, int, const char *, Int_t);
TBranch * TTree::Branch(const char *, Longptr_t, const char *, Int_t);

///.*operator new.*/;
///.*operator delete.*/;
//Version_t TObjArrayIter::Class_Version();
//const char * TObjArrayIter::DeclFileName();
///.*TObject::.*/;
