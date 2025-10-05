#include "TypeMapper.h"
#include "libclang-ext.h"

#include <iostream>
#include <sstream>
#include <regex>

extern int verbose;

bool TypeMapper::is_mapped(CXType t, bool as_return) const {
  return is_mapped(fully_qualified_name(t), as_return);
}

std::string TypeMapper::mapped_typename(CXType from, bool as_return,
                                        bool* ismapped) const{
  return mapped_type(fully_qualified_name(from), as_return, ismapped);
}


std::string TypeMapper::mapped_type(const std::string& from, bool as_return,
                                    bool* mapped) const{
  std::string to;
  mapped_type_impl(from, as_return, mapped, &to);
  return to;
}

template<typename T>
static bool in(const T& x, const std::vector<T> l){
  return std::find(l.begin(), l.end(), x) != l.end();
}

void TypeMapper::mapped_type_impl(const std::string& from, bool as_return,
                                  bool* mapped, std::string* to) const{
  //auto it = map_.find(from);
  bool isconst = false;
  auto it = std::find_if(map_.begin(), map_.end(),
                         [&isconst, from](const auto& x){
                           if(x.first == from){
                             return true;
                           } else if("const " + x.first == from){
                             isconst = true;
                             return true;
                           } else{
                             return false;
                           }
                         }
                         );
  if(it!= map_.end()){
    if(mapped) *mapped = true;
    const std::string& newtype = (as_return ? it->second.as_return : it->second.as_arg);
    auto already_const = std::regex_match(newtype, std::regex("^const .*"));
    //    std::cerr << "==> " << newtype << ", already const: " << already_const << "\n";
    if(to) *to = ((isconst && !already_const) ?
                  "const "
                  : "")
             + newtype;
  } else{
    if(mapped) *mapped = false;
    if(to) *to = from;
  }
}

bool TypeMapper::mutate(std::string& to_change, bool as_return){
  bool changed;
  mapped_type_impl(to_change, as_return, &changed, &to_change);
  return changed;
}

bool TypeMapper::is_mapped(const std::string& from, bool as_return) const{
  bool result;
  mapped_type_impl(from, as_return, &result, nullptr);
  //  std::cerr << "==> " << from << " is " << (result ? "" : "not ") << "mapped\n";
  return result;
}

