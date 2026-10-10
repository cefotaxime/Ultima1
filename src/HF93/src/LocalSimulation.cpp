
#include "LocalSimulation.h"
#include "KoreanText.h"
#include <algorithm>
#include <chrono>
#include <ctime>

namespace {
std::uint16_t ror16(std::uint16_t v,int n){
    n&=15;
    return static_cast<std::uint16_t>((v>>n)|(v<<(16-n)));
}

std::string hitActorLabel(const LocalActor& a,bool inTown){
    return 인물_이름_한글(a.tile,inTown);
}

}

void DosRng::seedFromDosTime(std::uint16_t saveSeedWord){
    // OUT.EXE 0xAD24 uses DOS int 21h/AH=2Ch: CH=hour, CL=minute,
    // DH=second, DL=hundredths. It shifts CH*2, CL*4, DH*8, adds CX+DX,
    // XORs 0x91EB, masks to 12 bits, takes mod 999, then adds PLAYER.U1 +0x32.
    using namespace std::chrono;
    const auto now=system_clock::now();
    const auto tt=system_clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm,&tt);
#else
    localtime_r(&tt,&tm);
#endif
    const auto ms=duration_cast<milliseconds>(now.time_since_epoch()).count();
    const int hundredths=static_cast<int>((ms%1000)/10);
    const std::uint16_t cx=static_cast<std::uint16_t>((((tm.tm_hour*2)&0xFF)<<8)|((tm.tm_min*4)&0xFF));
    const std::uint16_t dx=static_cast<std::uint16_t>((((tm.tm_sec*8)&0xFF)<<8)|(hundredths&0xFF));
    std::uint16_t v=static_cast<std::uint16_t>(cx+dx);
    v=static_cast<std::uint16_t>((v^0x91EBu)&0x0FFFu);
    state_=static_cast<std::uint16_t>(saveSeedWord + (v%999u));
}

int DosRng::range(int minValue,int maxValue){
    // OUT.EXE 0xB4C3. max is capped to 255. The reduction loop is intentionally
    // subtraction rather than %, preserving the original inclusive [min,max] behavior.
    state_=ror16(static_cast<std::uint16_t>(state_+0x9248u),3);
    int v=state_&0x03FFu;
    maxValue=std::min(maxValue,255);
    const int d=maxValue-minValue;
    if(d<=0) return minValue&0xFF;
    while(v>d) v-=d;
    return (v+(minValue&0xFF))&0xFF;
}

int LocalSimulation::actorIndexAt(const LocalState& local,int x,int y) const{
    for(int i=0;i<OriginalData::LOCAL_ACTOR_COUNT;++i){
        const auto& a=local.actors[static_cast<std::size_t>(i)];
        if(a.active() && static_cast<int>(a.x)==x && static_cast<int>(a.y)==y) return i;
    }
    return -1;
}

const LocalActor* LocalSimulation::actorAt(const LocalState& local,int x,int y) const{
    const int i=actorIndexAt(local,x,y);
    return i<0?nullptr:&local.actors[static_cast<std::size_t>(i)];
}

std::uint8_t LocalSimulation::rawTile(const LocalState& local,int x,int y) const{
    if(x<0||x>=OriginalData::LOCAL_W||y<0||y>=OriginalData::LOCAL_H) return 0x3F;
    if(local.tilesInitialized) return local.tiles[static_cast<std::size_t>(x*OriginalData::LOCAL_H+y)];
    return original_.localRawTile(local.mapIndex,x,y);
}

int LocalSimulation::displayTile(const LocalState& local,int x,int y) const{
    const auto raw=rawTile(local,x,y);
    return raw>=0x33 ? 1 : static_cast<int>(raw);
}

bool LocalSimulation::passable(const LocalState& local,int x,int y) const{
    if(x<0||x>=OriginalData::LOCAL_W||y<0||y>=OriginalData::LOCAL_H) return false;
    const auto raw=rawTile(local,x,y);
    return raw==1 || raw>=0x33;
}

int LocalSimulation::queryCell(const LocalState& local,int x,int y) const{
    // Models OUT.EXE 0x23E7: bounds; player/actor occupancy; raw TCD semantics.
    if(x<0||x>=OriginalData::LOCAL_W||y<0||y>=OriginalData::LOCAL_H) return 0x3F;
    if(local.x==x && local.y==y) return 0x12;
    const int ai=actorIndexAt(local,x,y);
    if(ai>=0) return local.actors[static_cast<std::size_t>(ai)].tile;
    const auto raw=rawTile(local,x,y);
    if(raw==0x3F) return 0x3F;
    if(raw==1 || raw>=0x33) return -1;
    return 0;
}

int LocalSimulation::randomStep(){
    // OUT.EXE 0x2203: rng(1,215) % 3, mapping remainder 2 to -1.
    const int rem=rng_.range(1,215)%3;
    return rem==2?-1:rem;
}

bool LocalSimulation::tryMoveActor(LocalState& local,int actorIndex,int dx,int dy,bool allowExitTile){
    auto& a=local.actors[static_cast<std::size_t>(actorIndex)];
    const int nx=static_cast<int>(a.x)+dx;
    const int ny=static_cast<int>(a.y)+dy;
    if(nx<0||nx>=OriginalData::LOCAL_W||ny<0||ny>=OriginalData::LOCAL_H) return false;
    const int q=queryCell(local,nx,ny);
    if(q!=-1 && !(allowExitTile && q==0x3F)) return false;
    a.x=static_cast<std::uint16_t>(nx);
    a.y=static_cast<std::uint16_t>(ny);
    // OUT.EXE 0x27A4/0x28D4/0x2A5D/0x2B36: each successful
    // local actor move is immediately followed by sound(4).
    ++movementSoundCount_;
    return true;
}

void LocalSimulation::peacefulWander(LocalState& local,int actorIndex){
    if(local.alert) return;
    const int dx=randomStep();
    const int dy=randomStep();
    // Bard/Wench peaceful path requires query == -1 exactly; 0x3F is not accepted.
    tryMoveActor(local,actorIndex,dx,dy,false);
}

void LocalSimulation::bardOrJester(LocalState& local,PlayerSave& player,int actorIndex,std::vector<std::string>& out){
    if(local.alert) return;
    const int dx=randomStep();
    const int dy=randomStep();
    const auto& a=local.actors[static_cast<std::size_t>(actorIndex)];
    const int nx=static_cast<int>(a.x)+dx;
    const int ny=static_cast<int>(a.y)+dy;
    const int q=queryCell(local,nx,ny);
    bool stole=false;

    if(q==-1){
        tryMoveActor(local,actorIndex,dx,dy,false);
    }else if(q==0x12){
        // OUT.EXE 0x25B5 scans weapon slots 15..1, taking the highest-index
        // nonempty weapon that is not currently readied.
        for(int i=15;i>0;--i){
            if(player.weapons[static_cast<std::size_t>(i)]!=0 && i!=player.readyWeapon){
                --player.weapons[static_cast<std::size_t>(i)];
                stole=true;
                break;
            }
        }
        // Theft already happened; agility controls whether the player notices it.
        if(stole && rng_.range(1,255)<static_cast<int>(player.agility)+128){
            out.push_back(local.inTown?"이올로가 무언가를 훔쳤습니다!":"광대가 무언가를 훔쳤습니다!");
        }
    }

    if(!stole && rng_.range(1,255)<15){
        // M124: keep the exact actor-update/RNG point, but defer only the
        // random speech presentation. Theft notices remain immediate because
        // they are gameplay feedback, not ambient/random dialogue.
        if(local.inTown){
            delayedActorDialogue_.push_back("음유시인 이올로가 노래합니다: 호 이요 히 험!");
        }else{
            delayedActorDialogue_.push_back("광대 그웨노가 노래합니다: 내가 열쇠를 가졌네!");
        }
    }
}

void LocalSimulation::princess(LocalState& local,int actorIndex){
    if(!local.alert){
        peacefulWander(local,actorIndex);
        return;
    }
    auto& a=local.actors[static_cast<std::size_t>(actorIndex)];
    int dx=static_cast<int>(a.x)-local.x;
    int dy=static_cast<int>(a.y)-local.y;
    dx=(dx>0)?-1:(dx<0)?1:0;
    dy=(dy>0)?-1:(dy<0)?1:0;

    // OUT.EXE 0x27DD: rng(1,100)<50 chooses Y-first; otherwise X-first.
    if(rng_.range(1,100)<50){
        if(dy && tryMoveActor(local,actorIndex,0,dy,true)) return;
        if(dx) tryMoveActor(local,actorIndex,dx,0,true);
    }else{
        if(dx && tryMoveActor(local,actorIndex,dx,0,true)) return;
        if(dy) tryMoveActor(local,actorIndex,0,dy,true);
    }
}

bool LocalSimulation::guardChase(LocalState& local,int actorIndex){
    auto& a=local.actors[static_cast<std::size_t>(actorIndex)];
    int rawDx=static_cast<int>(a.x)-local.x;
    int rawDy=static_cast<int>(a.y)-local.y;
    const int ax=std::abs(rawDx), ay=std::abs(rawDy);
    if(ax<2 && ay<2) return false; // already adjacent -> queued for attack phase
    if(ax+ay>=13) return false;    // outside original pursuit radius
    const int dx=(rawDx>0)?-1:(rawDx<0)?1:0;
    const int dy=(rawDy>0)?-1:(rawDy<0)?1:0;
    if(dx && tryMoveActor(local,actorIndex,dx,0,true)) return true;
    if(dy && tryMoveActor(local,actorIndex,0,dy,true)) return true;
    return false;
}

void LocalSimulation::guardAttack(LocalState& local,PlayerSave& player,int actorIndex,std::vector<std::string>& out){
    const auto& a=local.actors[static_cast<std::size_t>(actorIndex)];
    if(std::abs(static_cast<int>(a.x)-local.x)>=2 || std::abs(static_cast<int>(a.y)-local.y)>=2) return;

    const int roll=rng_.range(1,255);
    const int evade=static_cast<int>(player.stamina)/2 + static_cast<int>(player.readyArmour)*8 + 56;
    if(roll<evade){
        guardAttackHits_.push_back(false);
        if(local.inTown){
            out.push_back("경비병의 공격!");
            out.push_back("공격이 빗나갑니다!");
        }else{
            // One castle retaliation is one logical stream.  The player attack
            // that triggered it is already a separate row, so this naturally
            // creates the requested player/enemy line break.
            out.push_back("경비병의 공격! 공격이 빗맞습니다!");
        }
        return;
    }

    guardAttackHits_.push_back(true);
    const int upper=static_cast<int>(player.hits)/128 + 15;
    const int damage=rng_.range(2,upper);
    if(local.inTown){
        out.push_back("경비병의 공격!");
        out.push_back("적중!... "+std::to_string(damage)+"의 피해량!");
    }else{
        out.push_back("경비병의 공격! 공격 적중!..."+
                      std::to_string(damage)+"의 피해량!");
    }
    player.hits=static_cast<std::uint16_t>(damage>=player.hits?0:player.hits-damage);
}

bool LocalSimulation::princessAdjacent(const LocalState& local) const{
    if(!local.active || local.inTown) return false;
    // OUT.EXE 0x269D checks all eight neighbours around the player for tile 0x16.
    for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx){
        if(dx==0 && dy==0) continue;
        const int x=local.x+dx, y=local.y+dy;
        if(x<0||y<0||x>=OriginalData::LOCAL_W||y>=OriginalData::LOCAL_H) continue;
        const int ai=actorIndexAt(local,x,y);
        if(ai>=0 && local.actors[static_cast<std::size_t>(ai)].tile==0x16) return true;
    }
    return false;
}

std::vector<std::string> LocalSimulation::rescuePrincess(LocalState& local,PlayerSave& player){
    std::vector<std::string> out;
    if(!princessAdjacent(local)) return out;

    // OUT.EXE 0x4DD6 checks the extra-gift condition before applying the +500 reward.
    const bool extraGift=(player.spaceKills>=20u && player.experience>=7000u);
    const std::string princess=공주_이름_한글(original_.princessNameForCastle(local.locationId));
    out.push_back("당신은 "+princess+" 공주를 구출하였습니다.");
    out.push_back("그녀가 당신에게");
    out.push_back("생명력과 코인, 경험치를");
    out.push_back("각각 500씩 줍니다.");

    player.coin=static_cast<std::uint16_t>(std::min(9999,static_cast<int>(player.coin)+500));
    player.hits=static_cast<std::uint16_t>(std::min(9999,static_cast<int>(player.hits)+500));
    player.experience=static_cast<std::uint16_t>(std::min(9999,static_cast<int>(player.experience)+500));

    if(!extraGift) return out;
    // M139: user-approved Korean translation of the Time-Machine guidance.
    out.push_back("그녀는 당신이 이제 시간 여행을 할");
    out.push_back("준비가 되었으며, 타임머신은 여기서");
    out.push_back("북서쪽으로 먼 곳에 있다고");
    out.push_back("말해줍니다!");

    // PLAYER +0xA4 is transports[7], the Time Machine inventory count.
    if(player.transports[7]<9u) ++player.transports[7];

    // 0x4EE9..0x4F9F: choose this continent's northwest search box and make
    // up to 500 attempts for raw terrain class 2 (woods). c4af excludes the
    // player record by passing startIndex=1, so any existing world object blocks it.
    const int baseX=(player.x>83u)?83:0;
    const int baseY=(player.y>77u)?77:0;
    auto worldObjectAt=[&](int x,int y)->bool{
        for(int i=1;i<OriginalData::WORLD_OBJECT_COUNT;++i){
            const auto& o=player.worldObjects[static_cast<std::size_t>(i)];
            if(o.empty()) continue;
            if(static_cast<int>(o.x())==x && static_cast<int>(o.y())==y) return true;
        }
        return false;
    };
    auto append=[&](int x,int y)->bool{
        int last=0;
        for(int i=OriginalData::WORLD_OBJECT_COUNT-1;i>0;--i){
            if(!player.worldObjects[static_cast<std::size_t>(i)].empty()){last=i;break;}
        }
        if(last>=OriginalData::WORLD_OBJECT_COUNT-1) return false;
        auto& o=player.worldObjects[static_cast<std::size_t>(last+1)];
        o.w.fill(0); o.w[0]=0x12; o.w[1]=2; o.w[2]=static_cast<std::uint16_t>(x); o.w[3]=static_cast<std::uint16_t>(y);
        return true;
    };
    for(int attempt=0;attempt<500;++attempt){
        const int x=baseX+rng_.range(12,21);
        const int y=baseY+rng_.range(11,15);
        if(x<0||y<0||x>=OriginalData::MAP_W||y>=OriginalData::MAP_H) continue;
        if(worldObjectAt(x,y)) continue;
        if(original_.terrainAt(x,y)!=2) continue;
        append(x,y);
        break;
    }
    return out;
}

std::vector<std::string> LocalSimulation::updateAfterCommand(LocalState& local,PlayerSave& player){
    delayedActorDialogue_.clear();
    movementSoundCount_=0;
    guardAttackHits_.clear();
    std::vector<std::string> out;
    if(!local.active) return out;
    std::array<int,OriginalData::LOCAL_ACTOR_COUNT> attackGuards{};
    int guardCount=0;

    for(int i=0;i<OriginalData::LOCAL_ACTOR_COUNT;++i){
        auto& a=local.actors[static_cast<std::size_t>(i)];
        if(!a.active()) continue;
        switch(a.tile){
            case 0x13: bardOrJester(local,player,i,out); break;
            case 0x32: peacefulWander(local,i); break;
            case 0x16: princess(local,i); break;
            case 0x11:
                if(local.alert && !guardChase(local,i)) attackGuards[static_cast<std::size_t>(guardCount++)]=i;
                break;
            default: break; // King and Merchant are stationary.
        }
    }

    if(local.alert){
        for(int n=0;n<guardCount && player.hits>0;++n)
            guardAttack(local,player,attackGuards[static_cast<std::size_t>(n)],out);
    }
    return out;
}

std::vector<std::string> LocalSimulation::takeDelayedActorDialogue(){
    auto out=std::move(delayedActorDialogue_);
    delayedActorDialogue_.clear();
    return out;
}

std::vector<std::string> LocalSimulation::attack(LocalState& local,PlayerSave& player,int dx,int dy){
    std::vector<std::string> out;
    lastPlayerAttackHit_=false;
    if(!local.active) return out;
    const int weapon=std::clamp<int>(player.readyWeapon,0,15);
    const int range=OriginalData::weaponRange(weapon);
    if(range<=0){ out.push_back("대상이 없습니다"); return out; }
    const int hitChance=static_cast<int>(player.agility)+50;
    const int damage=rng_.range(2,static_cast<int>(player.strength)+weapon*8);

    int target=-1;
    for(int dist=1;dist<=range;++dist){
        const int tx=local.x+dx*dist, ty=local.y+dy*dist;
        target=actorIndexAt(local,tx,ty);
        if(target>=0) break;
        const int q=queryCell(local,tx,ty);
        if(q!=-1 && q!=0x3F) break;
    }
    if(target<0 || rng_.range(1,100)>hitChance){
        out.push_back(local.inTown?"공격이 빗나갑니다!":"공격이 빗나갑니다!");
        return out;
    }

    auto& a=local.actors[static_cast<std::size_t>(target)];
    lastPlayerAttackHit_=true;
    const std::string castleTarget=local.inTown?std::string{}:std::string(인물_이름_한글(a.tile,false));
    local.alert=true; // OUT.EXE 0x2D4A

    if(damage>=static_cast<int>(a.value)){
        if(local.inTown){
            out.push_back(hitActorLabel(a,local.inTown)+"에게 적중!");
            out.push_back("처치했습니다!");
        }else{
            // Keep hit+damage together, but force the kill acknowledgement to
            // begin on the next command-window row.
            out.push_back(castleTarget+"에게 적중! "+
                          std::to_string(damage)+"의 피해량!");
            out.push_back("처치했습니다!");
        }
        if(a.tile!=0x16){
            // OUT.EXE 0x2F7A immediately calls the EXP display/cap helper,
            // then guards receive the separate +14 at 0x2FBC.
            player.experience=static_cast<std::uint16_t>(std::min(9999,static_cast<int>(player.experience)+1));
            if(a.tile==0x11) player.experience=static_cast<std::uint16_t>(
                std::min(9999,static_cast<int>(player.experience)+14));
        }
        if(a.tile==0x13 && !local.inTown){
            out.push_back("열쇠를 발견했습니다!");
            local.foundKey=true;
        }
        a.tile=0xFFFF;
        a.x=0x7F;
    }else{
        // OUT.EXE 0x2DDF: the King in every castle except location 33 ignores nonlethal damage.
        const bool kingIgnoresDamage=(a.tile==0x14 && local.locationId!=33);
        if(!kingIgnoresDamage) a.value=static_cast<std::uint16_t>(a.value-damage);
        if(local.inTown){
            out.push_back(hitActorLabel(a,local.inTown)+"에게 적중!");
            out.push_back(std::to_string(damage)+"의 피해량!");
        }else{
            out.push_back(castleTarget+"에게 적중! "+
                          std::to_string(damage)+"의 피해량!");
        }
    }
    return out;
}


std::vector<std::string> LocalSimulation::steal(LocalState& local,PlayerSave& player){
    std::vector<std::string> out{"훔치기"};
    if(!local.active){ out.push_back("뭐라고요?"); return out; }
    const auto marker=rawTile(local,local.x,local.y);
    if(marker!=0x37 && marker!=0x39 && marker!=0x3B){
        out[0]+=" - 여기는 훔칠 것이 없습니다!";
        return out;
    }

    // OUT.EXE file 0x5D8A..0x5DF9.  Existing local alert forces a catch.
    // Otherwise class 3 (Thief) succeeds for rolls >= 38, while every
    // other class succeeds for rolls > 77.  The previous M19-M39 port
    // incorrectly treated class 3 as the only class allowed to steal.
    const int roll=rng_.range(1,255);
    const bool caught = local.alert || (player.klass==3 ? roll<38 : roll<=77);
    if(caught){
        out.push_back("이런! 발각되었습니다!");
        local.alert=true;
        return out;
    }

    if(marker==0x39){
        const int bags=rng_.range(2,31);
        out.push_back("식량 자루 "+std::to_string(bags)+"개를 발견하였습니다!");
        player.food=static_cast<std::uint16_t>(
            std::min(9999,static_cast<int>(player.food)+bags));
        return out;
    }
    if(marker==0x37){
        static constexpr const char* ARMOUR[6]={"맨몸","가죽 갑옷","사슬 갑옷","판금 갑옷","우주복","반사복"};
        const int i=rng_.range(1,5);
        out.push_back(조사_처리기("["+std::string(ARMOUR[i])+"]@을 발견하였습니다!"));
        if(player.armour[static_cast<std::size_t>(i)]<255) ++player.armour[static_cast<std::size_t>(i)];
        return out;
    }
    static constexpr const char* WEAPONS[16]={
        "맨손","단검","철퇴","도끼","밧줄과 못","검","대검","활과 화살",
        "수호부","지팡이","마법봉","삼각검","권총","광선검","페이저","블라스터"};
    const int i=rng_.range(1,15);
    out.push_back(조사_처리기("["+std::string(WEAPONS[i])+"]@을 발견하였습니다!"));
    if(player.weapons[static_cast<std::size_t>(i)]<255) ++player.weapons[static_cast<std::size_t>(i)];
    return out;
}

std::vector<std::string> LocalSimulation::unlock(LocalState& local,PlayerSave&){
    const bool castle=local.active && !local.inTown;
    std::vector<std::string> out{"무슨 자물쇠를 엽니까?"};
    if(!local.active){ out.push_back("뭐라고요?"); return out; }
    const auto tile=rawTile(local,local.x,local.y);
    if(tile!=0x3C && tile!=0x3D){
        if(!castle && !local.inTown) out.push_back("뭐라고요?");
        return out;
    }
    if(!local.foundKey){ out.push_back("열쇠가 없습니다!"); return out; }
    if(tile!=local.correctKeyTile){ out.push_back("올바른 열쇠가 아닙니다!"); return out; }
    out.push_back("문이 열렸습니다!");
    if(local.tilesInitialized && local.y>0)
        local.tiles[static_cast<std::size_t>(local.x*OriginalData::LOCAL_H+(local.y-1))]=1;
    return out;
}


std::vector<std::string> LocalSimulation::get(LocalState& local,PlayerSave& player,int& allowance){
    const bool castle=local.active && !local.inTown;
    std::vector<std::string> out{"가지기"};
    // OUT.EXE 0x5750: only mode 2 (castle-local mode) reaches the storeroom helper.
    if(!local.active || local.inTown){
        if(local.active && local.inTown) out.push_back(" 무엇을?");
        out.push_back(" - 여기는 가질 것이 없습니다!");
        return out;
    }
    const auto marker=rawTile(local,local.x,local.y);
    if(marker!=0x37 && marker!=0x39 && marker!=0x3B){
        out[0]+=" - 여기는 가질 것이 없습니다!";
        return out;
    }
    // OUT.EXE 0x5692/56B5/56D8: permission is checked before the random item helper.
    if(allowance<=0){
        out.push_back("국왕이 윤허하지 않았습니다!");
        return out;
    }
    --allowance;
    if(marker==0x39){
        const int bags=rng_.range(2,31);
        out.push_back("발견하였습니다:");
        out.push_back("식량 자루 "+std::to_string(bags)+"개");
        player.food=static_cast<std::uint16_t>(
            std::min(9999,static_cast<int>(player.food)+bags));
        return out;
    }
    if(marker==0x37){
        static constexpr const char* ARMOUR[6]={"맨몸","가죽 갑옷","사슬 갑옷","판금 갑옷","우주복","반사복"};
        const int i=rng_.range(1,5);
        out.push_back("발견하였습니다:");
        out.push_back(ARMOUR[i]);
        if(player.armour[static_cast<std::size_t>(i)]<255) ++player.armour[static_cast<std::size_t>(i)];
        return out;
    }
    static constexpr const char* WEAPONS[16]={
        "맨손","단검","철퇴","도끼","밧줄과 못","검","대검","활과 화살",
        "수호부","지팡이","마법봉","삼각검","권총","광선검","페이저","블라스터"};
    const int i=rng_.range(1,15);
    out.push_back("발견하였습니다:");
    out.push_back(WEAPONS[i]);
    if(player.weapons[static_cast<std::size_t>(i)]<255) ++player.weapons[static_cast<std::size_t>(i)];
    return out;
}


std::vector<std::string> LocalSimulation::dropPence(LocalState& local,PlayerSave& player,int amount){
    std::vector<std::string> out;
    if(!local.active || amount<=0){ out.push_back("버렸습니다!"); return out; }
    const auto marker=rawTile(local,local.x,local.y);

    // OUT.EXE 0x1E39: town mode accepts the healer marker 0x33 only.
    if(local.inTown){
        if(marker!=0x33){ out.push_back("버렸습니다!"); return out; }
        const int heal=(amount*3)/2;
        player.hits=static_cast<std::uint16_t>(std::min(9999,static_cast<int>(player.hits)+heal));
        // 0x1E97: rare spell gift. Roll <=16, then spell 1..7 with 3 remapped to 4.
        if(rng_.range(1,255)<=16){
            int sp=rng_.range(1,7); if(sp==3) sp=4;
            if(player.spells[static_cast<std::size_t>(sp)]<255) ++player.spells[static_cast<std::size_t>(sp)];
            out.push_back("수리수리 마수리!");
        }else out.push_back("짜잔!");
        return out;
    }

    // Castle mode: three donation markers, OUT.EXE 0x1CE4.
    if(marker==0x33){
        const int stat=rng_.range(1,6);
        std::uint16_t* dst=nullptr;
        switch(stat){
            case 1: dst=&player.strength; break; case 2: dst=&player.agility; break;
            case 3: dst=&player.stamina; break; case 4: dst=&player.charisma; break;
            case 5: dst=&player.wisdom; break; default: dst=&player.intelligence; break;
        }
        *dst=static_cast<std::uint16_t>(std::min(99,static_cast<int>(*dst)+amount/10));
        out.push_back("짜잔!");
    }else if(marker==0x34){
        if(amount>=10){
            const int w=rng_.range(1,15);
            if(player.weapons[static_cast<std::size_t>(w)]<255) ++player.weapons[static_cast<std::size_t>(w)];
        }
        out.push_back("짜잔!");
    }else if(marker==0x35){
        player.food=static_cast<std::uint16_t>(
            std::min(9999,static_cast<int>(player.food)+amount*3));
        out.push_back("짜잔!");
    }else out.push_back("버렸습니다!");
    return out;
}
