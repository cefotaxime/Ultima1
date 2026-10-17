#include "CastleCommandText.h"
#include "OriginalData.h"

#include <iostream>
#include <string>
#include <vector>

[[noreturn]] static void fail(const std::string& s){
    std::cerr << "M123 castle Inform/search FAIL: " << s << "\n";
    std::exit(1);
}

int main(int argc,char** argv){
    if(argc!=2) fail("usage: test GAMEDATA");

    OriginalData original;
    std::string error;
    if(!original.loadGameplayOracle(argv[1],error))
        fail("could not load original gameplay oracle: "+error);

    const std::vector<std::string> expected={
        "로드 브리티시성",
        "잊힌 왕의 성",
        "바라타리아성",
        "론도린성",
        "올림푸스성",
        "흑룡성",
        "백룡성",
        "샤미노성",
    };

    int seen=0;
    for(const auto& loc:original.locations()){
        if(loc.kind!=LocationKind::Castle) continue;
        if(seen>=static_cast<int>(expected.size())) fail("too many castles");
        PlayerSave p;
        p.x=static_cast<std::uint16_t>(loc.x);
        p.y=static_cast<std::uint16_t>(loc.y);
        const std::string got=성_탐지_결과(original,p);
        if(got!=expected[static_cast<std::size_t>(seen)])
            fail("castle id "+std::to_string(loc.id)+" name mismatch: "+got);
        ++seen;
    }
    if(seen!=8) fail("expected eight castles");

    PlayerSave nowhere;
    nowhere.x=167; nowhere.y=155;
    if(!성_탐지_결과(original,nowhere).empty())
        fail("non-castle coordinate produced castle Inform result");

    std::cout << "M123 castle Inform/search name test OK\n";
    return 0;
}
