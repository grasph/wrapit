#ifndef FILETIMESTAMPRESTORER_H
#define FILETIMESTAMPRESTORER_H

#include <sys/stat.h>
#include <string>
#include <vector>

// Helper class to restore the time stamp
// of an edited file if its contents it not
// modified by the edition.
class FileTimeRestorer{
public:
  FileTimeRestorer(): nskiplines_(0), statsucceeded_(false){}
  FileTimeRestorer(const std::string& fname, int nskiplines = 0);
  bool settimestamp() const;
  
private:
  std::string fname_;
  int nskiplines_;
  std::vector<unsigned char> md5_;
  struct stat filestat_;
  bool statsucceeded_;
};
#endif //FILETIMESTAMPRESTORER_H not defined
