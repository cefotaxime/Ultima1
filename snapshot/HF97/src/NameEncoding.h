#pragma once
#include <string>
#include <string_view>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif
namespace NameEncoding {
#ifdef _WIN32
inline std::wstring decode(std::string_view s,unsigned page,unsigned flags=MB_ERR_INVALID_CHARS){
 if(s.empty())return {};
 int n=MultiByteToWideChar(page,flags,s.data(),(int)s.size(),nullptr,0);
 if(n<=0)return {};
 std::wstring w((size_t)n,L'\0');
 if(MultiByteToWideChar(page,flags,s.data(),(int)s.size(),&w[0],n)!=n)return {};
 return w;
}
inline std::string encode(std::wstring_view w,unsigned page){
 if(w.empty())return {};
 int n=WideCharToMultiByte(page,0,w.data(),(int)w.size(),nullptr,0,nullptr,nullptr);
 if(n<=0)return {};
 std::string s((size_t)n,'\0');
 if(WideCharToMultiByte(page,0,w.data(),(int)w.size(),&s[0],n,nullptr,nullptr)!=n)return {};
 return s;
}
inline std::string toCp949(std::string_view utf8){
 auto w=decode(utf8,CP_UTF8);
 if(w.empty()&&!utf8.empty())return {};
 // Check for unrepresentable characters by round trip.
 auto encoded=encode(w,949);
 if(decode(encoded,949,0)!=w)return {};
 return encoded;
}
inline std::string fromSave(std::string_view bytes){
 // Preserve existing UTF-8 name saves; new saves are explicitly CP949.
 auto asUtf8=decode(bytes,CP_UTF8);
 if(!asUtf8.empty())return std::string(bytes);
 return encode(decode(bytes,949,0),CP_UTF8);
}
#elif defined(__DJGPP__)
#include "dos_cp949_table.h"
inline void appendUtf8(std::string& out,unsigned code){
 if(code<0x80)out.push_back((char)code);
 else if(code<0x800){out.push_back((char)(0xC0|(code>>6)));out.push_back((char)(0x80|(code&63)));}
 else {out.push_back((char)(0xE0|(code>>12)));out.push_back((char)(0x80|((code>>6)&63)));out.push_back((char)(0x80|(code&63)));}
}
inline bool validUtf8(std::string_view bytes){
 for(size_t i=0;i<bytes.size();){unsigned c=(unsigned char)bytes[i];if(c<128){++i;continue;}
  unsigned n=c>=0xC2&&c<=0xDF?2:c>=0xE0&&c<=0xEF?3:c>=0xF0&&c<=0xF4?4:0;
  if(!n||i+n>bytes.size())return false;
  for(unsigned j=1;j<n;j++)if(((unsigned char)bytes[i+j]&0xC0)!=0x80)return false;
  if(n==3){unsigned q=(unsigned char)bytes[i+1];if((c==0xE0&&q<0xA0)||(c==0xED&&q>=0xA0))return false;}
  if(n==4){unsigned q=(unsigned char)bytes[i+1];if((c==0xF0&&q<0x90)||(c==0xF4&&q>=0x90))return false;}
  i+=n;
 }return true;
}
inline std::string fromSave(std::string_view bytes){
 // HF93 compatibility: historical UTF-8 save names must remain unchanged.
 if(validUtf8(bytes))return std::string(bytes);
 std::string out;out.reserve(bytes.size()*2);
 for(size_t i=0;i<bytes.size();){unsigned lead=(unsigned char)bytes[i];
  if(lead<128){out.push_back((char)lead);++i;continue;}
  if(i+1>=bytes.size()){appendUtf8(out,0xFFFD);break;}
  unsigned tail=(unsigned char)bytes[i+1];
  unsigned code=(lead>=0x81&&lead<=0xFE&&tail>=0x41&&tail<=0xFE)?kCp949ToHangul[(lead-0x81)*190+(tail-0x41)]:0;
  appendUtf8(out,code?code:0xFFFD);i+=2;
 }
 return out;
}
inline std::string toCp949(std::string_view utf8){return std::string(utf8);}
#else
inline std::string toCp949(std::string_view utf8){return std::string(utf8);}
inline std::string fromSave(std::string_view bytes){return std::string(bytes);}
#endif
inline size_t byteCount(std::string_view utf8){
 const auto s=toCp949(utf8);
 return s.empty()&&!utf8.empty()?1000:s.size();
}
}