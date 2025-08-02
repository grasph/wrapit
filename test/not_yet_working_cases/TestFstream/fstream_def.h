#include <string>
#include <fstream>

template class std::char_traits<char>;
template class std::char_traits<wchar_t>;


template class std::basic_ofstream<char>;
template class std::basic_ifstream<char>;
template class std::basic_fstream<char>;

template class std::basic_ofstream<wchar_t>;
template class std::basic_ifstream<wchar_t>;
template class std::basic_fstream<wchar_t>;

//namespace std{
//  std::basic_ostream<char, std::char_traits<char>>
//  operator<<(std::basic_ostream<char, std::char_traits<char>>& s, char c);
//  
//  std::basic_ostream<char, std::char_traits<char>>
//  operator<<(std::basic_ostream<char, std::char_traits<char>>& s, const char* c);
//}
