#include "uuid_utils.h"

#include <regex>

std::string gen_uuid(){
  const char* v = "0123456789abcdef";
  //3fb17ebc-bc38-4939-bc8b-74f2443281d4
  //8 dash 4 dash 4 dash 4 dash 12
  std::string s;
  s.resize(36);
  for(int i = 0; i < 36; ++i) {
    if(i != 8 && i != 13 && i != 18 && i != 23){
      s[i] = v[rand()%16];
    } else{
      s[i] = '-';
    }
  }
  return s;
}

bool validate_uuid(const std::string& uuid){
  static std::regex r("[0-9a-f]{8}(-[0-9a-f]{4}){3}-[0-9a-f]{12}",
                      std::regex::icase);
  return std::regex_match(uuid, r);
}
