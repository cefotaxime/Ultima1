#include "KoreanText.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

int main(){
    const std::vector<std::pair<std::string,std::string>> cases={
        {"The Castle of Lord British","로드 브리티시성"},
        {"The Castle of the Lost King","잊힌 왕의 성"},
        {"The Castle Barataria","바라타리아성"},
        {"The Castle Rondorin","론도린성"},
        {"The Castle of Olympus","올림푸스성"},
        {"The Black Dragon's Castle","흑룡성"},
        {"The White Dragon's Castle","백룡성"},
        {"The Castle of Shamino","샤미노성"},
    };

    for(const auto& [english,korean]:cases){
        const char* got=성_이름_한글(english);
        if(!got || std::string(got)!=korean){
            std::cerr<<"M103 castle name FAIL: "<<english<<"\n";
            return 1;
        }
        const std::string line=korean+"에 진입합니다...";
        if(line.find('\n')!=std::string::npos){
            std::cerr<<"M103 entry line unexpectedly contains newline\n";
            return 1;
        }
    }

    if(성_이름_한글("Not a castle")!=nullptr){
        std::cerr<<"M103 unknown castle should not invent translation\n";
        return 1;
    }

    std::cout<<"M103 castle name/entry-line test OK\n";
    return 0;
}