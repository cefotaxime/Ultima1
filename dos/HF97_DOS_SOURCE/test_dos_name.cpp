#include "NameEncoding.h"
#include <stdio.h>
#include <string>
int main(){
 std::string n=u8"가나다라마바사";
 auto encoded=NameEncoding::toCp949(n);
 auto decoded=NameEncoding::fromSave(encoded);
 FILE* f=fopen("NAMECHK.TXT","wb"); if(!f)return 2;
 fprintf(f,"UTF8_INPUT_BYTES=%u\nCP949_BYTES=%u\nROUNDTRIP=%d\nSEVEN_ALLOWED=%d\nEIGHT_REJECTED=%d\n",(unsigned)n.size(),(unsigned)encoded.size(),(decoded==n),NameEncoding::byteCount(n)<=14,NameEncoding::byteCount(u8"가나다라마바사아")>14);
 fclose(f);
 return (encoded.size()==14&&decoded==n)?0:1;
}