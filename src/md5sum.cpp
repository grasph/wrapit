#include <openssl/md5.h>
#include <fstream>
#include "md5sum.h"

void md5sum(std::vector<unsigned char>& md5, const std::string& filename,
            int nlineskip){
  std::ifstream file(filename, std::ifstream::binary);
  for(int iline = 0; iline < nlineskip; ++iline){
    int c = 0;
    while(file.good() && c !='\n'){
      c = file.get();
    }
    ++iline;
    if(c < 0) break;
  }
  
  MD5_CTX md5Context;
  MD5_Init(&md5Context);
  char buf[1024 * 16];
  while (file.good()) {
    file.read(buf, sizeof(buf));
    MD5_Update(&md5Context, buf, file.gcount());
  }
  md5.resize(MD5_DIGEST_LENGTH);
  MD5_Final(md5.data(), &md5Context);
}
