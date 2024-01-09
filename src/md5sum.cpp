#include <fstream>
#include "md5sum.h"
#include <openssl/evp.h>

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

  auto hash_algo = EVP_md5();
  auto md5Context = EVP_MD_CTX_new();

  //  MD5_Init(&md5Context);
  EVP_DigestInit_ex(md5Context, hash_algo, NULL);
  char buf[1024 * 16];
  while (file.good()) {
    file.read(buf, sizeof(buf));
     EVP_DigestUpdate(md5Context, buf, file.gcount());
  }
  md5.resize(EVP_MAX_MD_SIZE);
  
  unsigned length;
  EVP_DigestFinal_ex(md5Context, md5.data(), &length);
  md5.resize(length);
}
