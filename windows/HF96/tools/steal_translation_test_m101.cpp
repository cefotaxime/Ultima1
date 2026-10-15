#include "KoreanText.h"
#include "LocalSimulation.h"
#include "OriginalData.h"

#include <iostream>
#include <string>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M101 steal translation FAIL: "<<s<<"\n";
    std::exit(1);
}

LocalState makeLocal(std::uint8_t marker){
    LocalState local;
    local.active=true;
    local.inTown=false;
    local.tilesInitialized=true;
    local.tiles.fill(1);
    local.x=0;
    local.y=0;
    local.tiles[0]=marker;
    return local;
}

bool endsWith(const std::string& s,const std::string& suffix){
    return s.size()>=suffix.size() &&
           s.compare(s.size()-suffix.size(),suffix.size(),suffix)==0;
}
}

int main(){
    OriginalData original;
    LocalSimulation sim(original);
    PlayerSave p;
    p.klass=3; // Thief: high success rate, but we still search deterministic seeds.

    // Forced caught path.
    {
        auto local=makeLocal(0x37);
        local.alert=true;
        sim.seed(1);
        const auto out=sim.steal(local,p);
        if(out.size()!=2 ||
           out[0]!="훔치기" ||
           out[1]!="이런! 발각되었습니다!")
            fail("caught output mismatch");
    }

    // Food theft: exact single-line Korean result.
    {
        bool ok=false;
        for(int seed=1;seed<=512 && !ok;++seed){
            auto local=makeLocal(0x39);
            local.alert=false;
            PlayerSave q;
            q.klass=3;
            sim.seed(static_cast<std::uint32_t>(seed));
            const auto out=sim.steal(local,q);
            if(out.size()==2 && out[1]!="이런! 발각되었습니다!"){
                if(out[0]!="훔치기") fail("food command label");
                if(out[1].find("식량 자루 ")!=0 ||
                   out[1].find("개를 발견하였습니다!")==std::string::npos)
                    fail("food result wording");
                ok=true;
            }
        }
        if(!ok) fail("could not reach deterministic food-success path");
    }

    // Armour theft: must exercise [물건]@을 through 조사_처리기 and leave no
    // template syntax visible.
    {
        bool ok=false;
        for(int seed=1;seed<=512 && !ok;++seed){
            auto local=makeLocal(0x37);
            local.alert=false;
            PlayerSave q;
            q.klass=3;
            sim.seed(static_cast<std::uint32_t>(seed));
            const auto out=sim.steal(local,q);
            if(out.size()==2 && out[1]!="이런! 발각되었습니다!"){
                if(out[0]!="훔치기") fail("armour command label");
                if(!endsWith(out[1]," 발견하였습니다!"))
                    fail("armour result wording");
                if(out[1].find('@')!=std::string::npos ||
                   out[1].find('[')!=std::string::npos ||
                   out[1].find(']')!=std::string::npos)
                    fail("armour particle template leaked to output");
                ok=true;
            }
        }
        if(!ok) fail("could not reach deterministic armour-success path");
    }

    // Weapon theft: same particle/template contract.
    {
        bool ok=false;
        for(int seed=1;seed<=512 && !ok;++seed){
            auto local=makeLocal(0x3B);
            local.alert=false;
            PlayerSave q;
            q.klass=3;
            sim.seed(static_cast<std::uint32_t>(seed));
            const auto out=sim.steal(local,q);
            if(out.size()==2 && out[1]!="이런! 발각되었습니다!"){
                if(out[0]!="훔치기") fail("weapon command label");
                if(!endsWith(out[1]," 발견하였습니다!"))
                    fail("weapon result wording");
                if(out[1].find('@')!=std::string::npos ||
                   out[1].find('[')!=std::string::npos ||
                   out[1].find(']')!=std::string::npos)
                    fail("weapon particle template leaked to output");
                ok=true;
            }
        }
        if(!ok) fail("could not reach deterministic weapon-success path");
    }

    // Direct grammar examples used by the actual theft implementation.
    if(조사_처리기("[반사복]@을 발견하였습니다!")!="반사복을 발견하였습니다!")
        fail("받침 item particle");
    if(조사_처리기("[도끼]@을 발견하였습니다!")!="도끼를 발견하였습니다!")
        fail("no-batchim item particle");

    std::cout<<"M101 steal translation test OK\n";
    return 0;
}