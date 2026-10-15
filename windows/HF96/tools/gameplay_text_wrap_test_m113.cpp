#include "GameplayTextWrap.h"

#include <iostream>
#include <string>
#include <vector>

[[noreturn]] static void fail(const std::string& s){
    std::cerr<<"M113 gameplay text wrap FAIL: "<<s<<"\n";
    std::exit(1);
}

static int nominalMeasure(const std::string& s){
    int w=0;
    for(std::size_t i=0;i<s.size();){
        const unsigned char c=static_cast<unsigned char>(s[i]);
        if(c<0x80u){
            w+=8;
            ++i;
        }else{
            w+=16;
            i=gameplayUtf8Next(s,i);
        }
    }
    return w;
}

int main(){
    const std::string msg=
        "Dark knight가 공격합니다! 공격이 적중했습니다! 11의 피해를 입습니다!";

    const auto lines=wrapGameplayTextMeasured(msg,29*16,nominalMeasure);
    if(lines.size()!=2) fail("expected exactly two visual rows");
    if(lines[0]!="Dark knight가 공격합니다! 공격이 적중했습니다! 11의 피해를")
        fail("first wrapped row mismatch");
    if(lines[1]!="입습니다!")
        fail("second wrapped row mismatch");
    for(const auto& line:lines)
        if(nominalMeasure(line)>29*16) fail("wrapped row exceeds command width");

    // No text may disappear. Spaces at the wrap point are represented by the
    // row boundary, so joining with one space must restore the original.
    if(lines[0]+" "+lines[1]!=msg)
        fail("wrapped text lost content");

    // A long Korean token without spaces must still hard-wrap only at UTF-8
    // code-point boundaries.
    const std::string korean=
        "가나다라마바사아자차카타파하가나다라마바사아자차카타파하";
    const auto hard=wrapGameplayTextMeasured(korean,8*16,nominalMeasure);
    if(hard.size()<2) fail("long Korean token did not wrap");
    std::string joined;
    for(const auto& line:hard){
        if(nominalMeasure(line)>8*16) fail("hard-wrapped Korean row exceeds width");
        joined+=line;
    }
    if(joined!=korean) fail("UTF-8 hard wrap lost bytes");

    std::cout<<"M113 gameplay text wrap test OK\n";
    return 0;
}