#include "KoreanText.h"
#include "ZtatsModel.h"

#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr << "M133 localization FAIL: " << s << "\n";
    std::exit(1);
}
const ZtatsItem& find(const std::vector<ZtatsItem>& v,const std::string& label){
    for(const auto& x:v) if(x.label==label) return x;
    fail("missing item: "+label);
}
}

int main(){
    static constexpr const char* T[6]={"말","마차","뗏목","범선","비행선","우주 왕복선"};
    for(int i=1;i<=6;++i) if(std::string(운송수단_이름_한글(i))!=T[i-1]) fail("transport name "+std::to_string(i));
    static constexpr const char* G[4]={"붉은색 보석","녹색 보석","푸른색 보석","흰색 보석"};
    for(int i=0;i<4;++i) if(std::string(보석_이름_한글(i))!=G[i]) fail("gem name "+std::to_string(i));

    PlayerSave p;
    for(int i=1;i<=6;++i) p.transports[static_cast<std::size_t>(i)]=1;
    for(int i=0;i<4;++i) p.gems[static_cast<std::size_t>(i)]=1;
    p.transport=5;
    const auto v=buildZtatsItems(p);
    for(const char* name:T) if(find(v,name).value!=1) fail(std::string("inventory transport ")+name);
    for(const char* name:G) if(find(v,name).value!=1) fail(std::string("inventory gem ")+name);
    if(!find(v,"비행선").equipped) fail("equipped Aircar localization");

    std::cout << "M133 requested localization regression OK\n";
    return 0;
}
