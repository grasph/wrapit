#ifndef TEMPLATES_H
#define TEMPLATES_H

//#include "TTree.h"
//#include "TH1.h"
//typedef TVectorT<double> TVectorD;

class TDirectoryFile;
class TH1;
class TTree;
#include "TTreeReaderValue.h"
#include "TTreeReaderArray.h"
#include "Rtypes.h"

//TObject  TFileDirectory::Get(const char *namecycle) override;
//TTree* GetTTree(TDirectoryFile* f, const char *namecycle);
//TH1* GetTH1(TDirectoryFile* f, const char *namecycle);

template class TTreeReaderValue<Char_t>;
template class TTreeReaderValue<UChar_t>;
template class TTreeReaderValue<Short_t>;
template class TTreeReaderValue<UShort_t>;
template class TTreeReaderValue<Int_t>;
template class TTreeReaderValue<UInt_t>;
template class TTreeReaderValue<Long_t>;
template class TTreeReaderValue<ULong_t>;
template class TTreeReaderValue<Float_t>;
template class TTreeReaderValue<Double_t>;
//template class TTreeReaderValue<LongDouble_t>;
//template class TTreeReaderValue<Bool_t>;
//template class TTreeReaderValue<Long64_t>;
//template class TTreeReaderValue<ULong64_t>;

//template class TTreeReaderArray<Char_t>;
//template class TTreeReaderArray<UChar_t>;
//template class TTreeReaderArray<Short_t>;
//template class TTreeReaderArray<UShort_t>;
//template class TTreeReaderArray<Int_t>;
//template class TTreeReaderArray<UInt_t>;
//template class TTreeReaderArray<Long_t>;
//template class TTreeReaderArray<ULong_t>;
//template class TTreeReaderArray<Float_t>;
//template class TTreeReaderArray<Double_t>;
//template class TTreeReaderArray<LongDouble_t>;
//template class TTreeReaderArray<Bool_t>;
//template class TTreeReaderArray<Long64_t>;
//template class TTreeReaderArray<ULong64_t>;

#endif //TEMPLATES_H not defined
