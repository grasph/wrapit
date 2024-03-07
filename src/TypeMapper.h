#ifndef TYPEMAPPER_H
#define TYPEMAPPER_H

#include <clang-c/Index.h>

#include <string>
#include <tuple>
#include <map>
#include <iostream> //TOREMOVE
class TypeMapper{
public:
  struct Spec{
    std::string as_arg;
    std::string as_return;
    Spec() = default;
    Spec(const std::string& as_arg, const std::string as_return):
      as_arg(as_arg), as_return(as_return){}
  };

  TypeMapper(){}

  TypeMapper& add(const std::string& from,
                  const std::string& arg_to,
                  const std::string& return_to){
    map_[from] = Spec(arg_to, return_to);
    return *this;
  }

  std::string mapped_type(const std::string& from, bool as_return = false,
                          bool* mapped = nullptr) const{
    auto it = map_.find(from);

    if(it==map_.end()){
      if(mapped) *mapped  = false;
      return from;
    } else{
      const std::string& to = as_return ? it->second.as_return : it->second.as_arg;
      if(mapped) *mapped = (to != from);
      return to;
    }
  }

  std::string mapped_typename(CXType from, bool as_return = false,
                              bool* mapped = nullptr) const;

  bool mutate(std::string& to_change, bool as_return = false){
    auto it = map_.find(to_change);
    if(it == map_.end()){
      return false;
    } else{
      std::string& new_type = as_return ? it->second.as_return :  it->second.as_arg;
      if(to_change != new_type){
        to_change = new_type;
        return true;
      } else{
        return false;
      }
    }
  }

  bool is_mapped(const std::string& from, bool as_return = false) const{
    auto it =  map_.find(from);
    if(it == map_.end()) return false;
    auto to = as_return ? it->second.as_return : it->second.as_arg;
    return to != from;
  }

  bool is_mapped(CXType type, bool as_return = false) const;

private:
  std::map<std::string, Spec> map_;
};

#endif //TYPEMAPPER_H not defined
