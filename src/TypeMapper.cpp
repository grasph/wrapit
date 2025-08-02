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

/////void TypeMapper::mapped_type_impl(const std::string& from, bool as_return,
/////                                  bool* mapped, std::string* to) const{
/////  const static bool trace = false;
/////
/////  //beware the call to this function from the mutate requires the algorithm
/////  //to work with to == &from.
/////
/////  //The algorithm must make the substitution sequentially starting
/////  //from the first element of map_
/////  // FIXME: modify the algorithm to not run again on the part
/////  // that were already modified.
/////  std::string str = from;
/////  std::stringstream buf;
/////  std::vector<char> boundaries = {'<', '>', ' ', '*', '&', '[', ']'};
/////  if(mapped) *mapped = false;
/////  if(trace) std::cerr << "==> Map type search for " << str << "\n";
/////  // loop on pattern starting from the longest, which must take precedence
/////  // E.g. ULong_t must take precedence on Long_t.
/////  for(auto it = map_.rbegin(); it != map_.rend(); ++it){
/////    auto m = *it;
/////    if(m.first.size() < 1) continue;
/////    if(trace) std::cerr << "\t==> Searching " << m.first << " in "
/////                      << str << "\n";
/////    std::string::size_type pos0 = 0;
/////    while(pos0 < str.size()){
/////      if(trace) std::cerr << "\t==> Search starting at position " << pos0 << "("
/////                        << str[pos0] << "...)\n";
/////      auto pos1 = str.find(m.first, pos0);
/////      auto pos2 = pos1 + str.size();
/////      if(pos1 != std::string::npos
/////         && (pos1 == 0 || in(str[pos1-1], boundaries))
/////         && (pos2 == str.size() || in(str[pos2], boundaries))){
/////        if(trace) std::cerr << "\t==> Found at position " << pos1 << "\n";
/////        if(mapped) *mapped = true;
/////        if(trace) std::cerr << "\t==> mapped: " << *mapped << "\n";
/////        if(mapped && !to){
/////          if(trace) std::cerr << "\t==> result: mapped to: N/A (early stop)\n"; 
/////          return;
/////        }
/////        if(trace) std::cerr << "\t==> Old buffer content: " << buf.str() << "\n";
/////        if(trace) std::cerr << "\t==> Copy to buffer '" << str.substr(pos0, pos1-pos0)
/////                          << "' and '" << (as_return ? m.second.as_return : m.second.as_arg)
/////                          << "'\n";
/////        if(pos1 > pos0) buf << str.substr(pos0, pos1 - pos0);
/////        buf << (as_return ? m.second.as_return : m.second.as_arg);
/////        if(trace)          std::cerr << "\t==> New buffer content: " << buf.str() << "\n";
/////        pos0 = pos1 + m.first.size();
/////      } else {
/////        if(trace) std::cerr << "\t==> Tail...\n";
/////        buf << str.substr(pos0);
/////        if(trace) std::cerr << "\t==> New buffer content: " << buf.str() << "\n";
/////        pos0 = std::string::npos;
/////      }
/////    }
/////    str = buf.str();
/////    buf.str("");
/////    if(trace) std::cerr << "\t==> Buffer content, '" << str << "' transferred to str. buf:"
/////                        << buf.str() << "\n";
/////  }
/////  if(to) *to = str;
/////  if(trace){
/////    std::cerr << "\t==> result: "
/////      "mapped = " << (mapped ? (*mapped ? "yes" : "no") : "N/A")
/////              << "; to = " << (to ? *to : "N/A") << "\n";
/////  }
/////}

//  auto toks = tokenize(from);
//
//  std::stringstream buf;
//  for(const auto& tok: toks){
//    if(tok.type == DELIM){
//      if(to) buf << tok.value;
//    } else{
//      auto it = map_.find(tok.value);
//      if(it != map_.end()){
//        if(mapped){
//          *mapped = true;
//          if(!to) break;
//        }
//        if(to){
//          buf << (as_return ? it->second.as_return : it->second.as_arg);
//        }
//      } else{
//        buf << tok.value;
//      }
//    }
//  }
  
//if(to) *to = buf.str();
//}

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

//std::vector<TypeMapper::Token> TypeMapper::tokenize(const std::string& str) {
//  std::vector<Token> tokens;
//
//  std::string::size_type pos0 = 0;
//  std::string::size_type pos1 = 0;
//
//  while(pos0 < str.size()){
//    pos1 = str.find_first_of(" <>*&[]", pos0);
//    if(pos1 != std::string::npos){
//      tokens.push_back(Token{TokenType::TOKEN, str.substr(pos0, pos1-pos0)});
//      tokens.push_back(Token{TokenType::DELIM, str.substr(pos1, 1)});
//      pos0 = pos1 + 1;
//    } else{//the tail
//      tokens.push_back(Token{TokenType::DELIM, str.substr(pos0)});
//      break;
//    }
//  }
//
//  return tokens;
//}
