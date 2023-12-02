#include "FileTimeRestorer.h"

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "md5sum.h"

#include <iostream>

extern int verbose;

FileTimeRestorer::FileTimeRestorer(const std::string& fname, int nskiplines):
  fname_(fname), nskiplines_(nskiplines){
  md5sum(md5_, fname_, nskiplines_);
  statsucceeded_ = (stat(fname_.c_str(), &filestat_) == 0);
}


bool FileTimeRestorer::settimestamp() const{
  if(!statsucceeded_) return false;
  std::vector<unsigned char> newmd5;
  md5sum(newmd5, fname_, nskiplines_);  
  if(newmd5 == md5_){
    if(verbose > 4){
      std::cerr << "Code in file " << fname_
                << " did not change, preserving the file timestamp.\n";
    }
    //restore time stamp:
    utimensat(AT_FDCWD, fname_.c_str(), &(filestat_.st_atim), 0);
    return true;
  } else{
    return false;
  }
}
