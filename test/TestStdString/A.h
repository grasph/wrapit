#include <string>
#include <cstdlib>

struct String: public std::string {
  String(const std::string& x): std::string(x){}
};

char* getenv(const char* name);

char* getenvstd(const std::string& x){
  return getenv(x.c_str());
}

int setenvstd(const std::string& name, const std::string& value){
  return setenv(name.c_str(), value.c_str(), 1);
}

char* getenvstd_1(std::string x){
  return getenv(x.c_str());
}

char* getenvstd_2(const std::string x){
  return getenv(x.c_str());
}

char* getenvstd_3(std::string& x){
  return getenv(x.c_str());
}

std::string getenvstd_std(const std::string& x){
  return std::string(getenv(x.c_str()));
}

String getenvString(const String& x){
  return std::string(getenv(x.c_str()));
}

