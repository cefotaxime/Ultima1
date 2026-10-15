#include "KoreanText.h"

#include <iostream>
#include <string>
#include <vector>

[[noreturn]] static void fail(const std::string& s){
    std::cerr<<"M111 Attack translation FAIL: "<<s<<"\n";
    std::exit(1);
}

int main(){
    const std::vector<std::string> expected={
        "맨손으로 공격하기",
        "단검으로 공격하기",
        "철퇴로 공격하기",
        "도끼로 공격하기",
        "밧줄과 못으로 공격하기",
        "검으로 공격하기",
        "대검으로 공격하기",
        "활과 화살로 공격하기",
        "수호부로 공격하기",
        "지팡이로 공격하기",
        "마법봉으로 공격하기",
        "삼각검으로 공격하기",
        "권총으로 공격하기",
        "광선검으로 공격하기",
        "페이저로 공격하기",
        "블라스터로 공격하기",
    };

    for(int i=0;i<16;++i){
        const std::string got=무기_공격_문구_한글(i);
        if(got!=expected[static_cast<std::size_t>(i)])
            fail(std::to_string(i)+": "+got);
    }

    // Explicitly lock the previously problematic Dagger wording.
    if(무기_공격_문구_한글(1)!="단검으로 공격하기")
        fail("Dagger special-case wording returned");
    if(무기_공격_문구_한글(15)!="블라스터로 공격하기")
        fail("Blaster not localized");

    std::cout<<"M111 all-weapon Attack phrase test OK\n";
    return 0;
}