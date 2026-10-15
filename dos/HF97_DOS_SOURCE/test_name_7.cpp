#include <iostream>
#include <string>
#include "NameEncoding.h"
int main(){
 const std::string seven=u8"가나다라마바사";
 const std::string eight=u8"가나다라마바사아";
 auto a=NameEncoding::toCp949(seven),b=NameEncoding::toCp949(eight);
 std::cout<<"SEVEN_BYTES="<<a.size()<<" EIGHT_BYTES="<<b.size()
          <<" SEVEN_ROUNDTRIP="<<(NameEncoding::fromSave(a)==seven)
          <<" LIMIT_SEVEN="<<(NameEncoding::byteCount(seven)<=14)
          <<" REJECT_EIGHT="<<(NameEncoding::byteCount(eight)>14)<<"\n";
 return a.size()==14 && b.size()==16 && NameEncoding::fromSave(a)==seven && NameEncoding::byteCount(eight)>14?0:1;
}