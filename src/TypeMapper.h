#ifndef TYPEMAPPER_H
#define TYPEMAPPER_H

#include <clang-c/Index.h>

#include <string>
#include <tuple>
#include <map>

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
    //map_.push_back(std::make_pair(from, Spec(arg_to, return_to)));
    return *this;
  }

  std::string mapped_type(const std::string& from, bool as_return = false,
                          bool* mapped = nullptr) const;

  std::string mapped_typename(CXType from, bool as_return = false,
                              bool* mapped = nullptr) const;

  void mapped_type_impl(const std::string& from, bool as_return,
                        bool* mapped, std::string* to) const;


  bool mutate(std::string& to_change, bool as_return = false);

  bool is_mapped(const std::string& from, bool as_return = false) const;
  bool is_mapped(CXType type, bool as_return = false) const;

private:
  //it is important that this is ordered according to string comparison
  //operator, such that the string are stored in increasing length.
  std::map<std::string, Spec> map_;
};

#endif //TYPEMAPPER_H not defined
