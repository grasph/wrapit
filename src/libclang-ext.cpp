// Copyright (C) 2021 Philippe Gras CEA/Irfu <philippe.gras@cern.ch>
#include "libclang-ext.h"
#include <regex>
#include <string>
#include <sstream>

#include "clang/AST/Decl.h"
#include "clang/AST/Type.h"
#include "clang/AST/PrettyPrinter.h"
#include "clang/AST/DeclCXX.h"
#include "clang/AST/DeclTemplate.h"
//#include "clang/Frontend/ASTUnit.h"
#include "llvm/Support/raw_ostream.h"

#include "utils.h"

int hasDefaultConstructor(CXCursor cursor){
  if(clang_isDeclaration(cursor.kind)){
    auto decl = static_cast<const clang::Decl*>(cursor.data[0]);
    auto cxx_record_decl =  llvm::dyn_cast<const clang::CXXRecordDecl>(decl);
    if(cxx_record_decl){
      return cxx_record_decl->hasDefaultConstructor() ? 1 : 0;
    }
  }
  return -1;
}


//struct CXTranslationUnitImpl {
//  void* CIdx;
//  clang::ASTUnit *TheASTUnit;
//  void *StringPool;
//  void *Diagnostics;
//  void *OverridenCursorsPool;
//  void *CommentToXML;
//  unsigned ParsingOptions;
//  std::vector<std::string> Arguments;
//};

std::string fully_qualified_name(CXCursor cursor) {
 if(clang_isDeclaration(cursor.kind)){
   auto decl = static_cast<const clang::Decl*>(cursor.data[0]);

//   auto tu = static_cast<const CXTranslationUnitImpl*>(cursor.data[2])->TheASTUnit;
//   
//   std::cerr << "==> typeid(*decl): " << typeid(*decl).name() << "\n";
//   auto tmp = llvm::dyn_cast<const clang::TypedefDecl>(decl);
//   if(tmp){
//     std::cerr << "==> !!!!\n";
//     auto tpsi = tmp->getTypeSourceInfo()->getType();
//     std::cerr << "type source info: " << tpsi.getAsString()
//	       << ", type: " << typeid(tpsi).name()
//	       << ", type: " << typeid(*tpsi.getTypePtr()).name()
//	       << ", iPOD: " << tpsi.isCXX11PODType(tu->getASTContext())
//	       << "\n";
//   }
   
   auto named_decl =  llvm::dyn_cast<const clang::NamedDecl>(decl);
   if(named_decl){
     std::string buffer_;
     llvm::raw_string_ostream buffer(buffer_);
     named_decl->printQualifiedName(buffer);
     return buffer.str();
   }
 }
 //if this point is reached, fully qualified name retrieval failed,
 //fall back to the original name:
 std::cerr  << "Warning: failed to retrieve the fully qualified name of " << cursor << "\n";
 return str(clang_getCursorSpelling(cursor));
}

//FIXME: see if the function below can't be implementd
//       using clang::QualType::getLocalUnqualifiedType

//Remove cv pointer and reference qualifiers of a type:
CXType base_type_(CXType type){
  auto type0 = type;
  CXType type1 = {CXType_Invalid, nullptr};
  while(!clang_equalTypes(type0, type1)){
    if(type0.kind == CXType_Pointer || type0.kind == CXType_LValueReference || type0.kind == CXType_RValueReference){
      type1 = type0;
      type0 = clang_getPointeeType(type0);
    } else if(type0.kind == CXType_Typedef){
      type1 = type0;
    } else{
      break;
    }
  }
  return type0;
}

std::string remove_cv(const std::string& type_name){
   char buf[type_name.size() + 1];
   strcpy(buf, type_name.c_str());
   char* saveptr;
   char* tok;
   char* p = buf;
   std::string result;
   std::string sep;
   while((tok = strtok_r(p, " \t", &saveptr)) != nullptr){
      p = nullptr;
      if(strcmp(tok, "const") != 0 && strcmp(tok, "volatile")){
	result += sep + tok;
	sep = " ";
      }
   }
   return result;
}

//FIXME: does not work for a template type returned
//       by clang_getArgType()
std::string fully_qualified_name(CXType t){
  // The fully qualified name spelling of a type can be obtained with
  // libclang using following trick:
  //
  // clang_getTypeSpelling(clang_getCursorType(clang_getTypeDeclaration(type))
  //
  // Nevertheless, it does not work for pointer or references.
  //
  // For such types, we extract the pointee type and use libclang
  // to get its local and fully qualified name.
  //
  // Then we substitute the local name of the pointee type
  // with the fully qualified name in the reference/pointer type
  // spelling.
  auto name = str(clang_getTypeSpelling(t));
  auto base = base_type_(t);

  //FQN of the "base type"
  std::string base_local_name;
  std::string base_fqn;
  auto tmpl= clang_getSpecializedCursorTemplate(clang_getTypeDeclaration(base));

  int nargs = -1;
  std::string tmpl_args;
  int nprocessed_args = 0;
  if(!clang_Cursor_isNull(tmpl)){
    //A templated type
    nargs = clang_Type_getNumTemplateArguments(base);
    std::vector<std::string> args_spelling(nargs);
    std::stringstream buf;
    std::string sep = "";
    //FIXME: if failed to retrieve an argument, extract if from the type spelling
    if(nargs > 0){
      buf << "<";
      for(decltype(nargs) iarg = 0; iarg < nargs; ++iarg){
	buf << sep;
	sep = ",";
	auto arg_type = clang_Type_getTemplateArgumentAsType (base, iarg);
	if(arg_type.kind != CXType_Invalid){
	  buf << fully_qualified_name(arg_type);
	  ++nprocessed_args;
	}
      }
      buf << ">";
      tmpl_args = buf.str();
    }

    //we first replace the type name withtout the <...> part with the spelling
    //found for tmpl. This step is needed because the type spelling in tmpl
    //can be different (e.g. w/o the namespace(s)) to the one in 'name'.
    auto base_local_name_0 = remove_cv(str(clang_getTypeSpelling(base)));
    base_local_name_0 = std::regex_replace(base_local_name_0, std::regex("<.*>"), "");
    base_local_name = remove_cv(str(clang_getCursorSpelling(tmpl)));
    replace(name, base_local_name_0,  base_local_name);
    //FQN of tmpl that will be used to substitute back the tmpl spelling in name:
    base_fqn = fully_qualified_name(tmpl);
  } else  if(base.kind > CXType_LastBuiltin){
    const auto& type_decl = clang_getTypeDeclaration(base);
    if(type_decl.kind != CXCursor_NoDeclFound){
      base_fqn = fully_qualified_name(clang_getTypeDeclaration(base));
    }
    base_local_name = remove_cv(str(clang_getTypeSpelling(base)));
  }

  auto fqn = name;
  if(base_fqn.size() > 0){
    replace(fqn, base_local_name, base_fqn);
  }

  if(nargs > 0 && nprocessed_args == nargs){
    fqn =    std::regex_replace(fqn, std::regex("<.*>"), tmpl_args);
  }

  if(nargs > 0 && nprocessed_args != nargs){
    std::cerr << "Warning: failed to interpret template argument of " << fqn
	      << ". Some argument might not be fully qualified in the produced "
      "c++ code, that is the code might need to be edited to add the possible "
      "namespace and class prefix.\n";
  }

  return fqn;
}


CXType remove_non_builtin_qualifiers(CXType type){
  //FIXME: there are probably more kinds to exclude
  if(type.kind > CXType_LastBuiltin && type.kind != CXType_FunctionProto){
    return clang_getCursorType(clang_getTypeDeclaration(type));
  } else{
    return type;
  }
}

CXType base_type(CXType type){
  return remove_non_builtin_qualifiers(base_type_(type));
}

std::vector<std::string>
get_template_parameters(CXCursor cursor){
  std::vector<std::string> res;
  auto decl = static_cast<const clang::Decl*>(cursor.data[0]);
  auto template_decl =  llvm::dyn_cast<const clang::TemplateDecl>(decl);
  if(!template_decl){
    return res;
  }

  clang::TemplateParameterList* params = template_decl->getTemplateParameters();
  for(clang::NamedDecl* p: *params){
    res.emplace_back(p->getNameAsString());
  }
  return res;
}
