#include "OverworldSimulation.h"
#include "KoreanText.h"
#include <algorithm>
#include <array>
#include <cstdlib>

namespace {
constexpr std::array<int,10> SPAWN_THRESHOLD={35,55,75,95,115,135,155,175,195,205};
constexpr std::array<int,15> SPAWN_STAT={4,6,8,10,1,2,4,6,8,2,4,6,8,9,10};
constexpr std::array<int,15> POWER={10,5,4,3,2,1,4,6,8,10,1,2,4,6,8};
constexpr std::array<int,8> MOVE_CHANCE={100,50,33,100,50,25,10,10};
constexpr std::array<const char*,15> NAMES={
    "네스호 괴수","거대 오징어","거북용","해적선","강도",
    "곰","은신 궁수","암흑 기사","사악한 나무 거인","도적","오크",
    "기사","강령술사","타락한 레인저","떠돌이 흑마술사"
};

int indexFromTile(int tile){
    if(tile<19 || tile>47 || ((tile-19)&1)) return -1;
    return (tile-19)/2;
}
}

bool OverworldSimulation::isMonsterTile(int tile){ return indexFromTile(tile)>=0; }
const char* OverworldSimulation::monsterName(int tile){
    const int i=indexFromTile(tile); return i>=0?NAMES[static_cast<std::size_t>(i)]:"Monster";
}
int OverworldSimulation::monsterPower(int tile){
    const int i=indexFromTile(tile); return i>=0?POWER[static_cast<std::size_t>(i)]:1;
}
int OverworldSimulation::monsterRange(int tile){
    // OUT.EXE 0x8194: only these four physical monster tiles have range 3.
    return (tile==0x17 || tile==0x19 || tile==0x1F || tile==0x2F) ? 3 : 1;
}

int OverworldSimulation::wrapX(int x){ x%=OriginalData::MAP_W; if(x<0)x+=OriginalData::MAP_W; return x; }
int OverworldSimulation::wrapY(int y){ y%=OriginalData::MAP_H; if(y<0)y+=OriginalData::MAP_H; return y; }
int OverworldSimulation::quadrant(int x,int y){ return (y>77?2:0)+(x>83?1:0); }
int OverworldSimulation::sign(int v){ return v>0?1:(v<0?-1:0); }

bool OverworldSimulation::seamXOk(int x){
    // OUT.EXE 0x7EB3. Coordinates in the second 84-wide half are normalized
    // by subtracting 84 before the range test.
    if(x>83) x-=84;
    return x<159 && x>8;
}
bool OverworldSimulation::seamYOk(int y){
    // OUT.EXE 0x7EE1. Same idea for the two 78-row halves.
    if(y>77) y-=78;
    return y<150 && y>5;
}

int OverworldSimulation::lastObject(const PlayerSave& p) const{
    for(int i=OriginalData::WORLD_OBJECT_COUNT-1;i>0;--i)
        if(!p.worldObjects[static_cast<std::size_t>(i)].empty()) return i;
    return 0;
}
int OverworldSimulation::objectAt(const PlayerSave& p,int x,int y,int startIndex) const{
    const int last=lastObject(p);
    for(int i=std::max(0,startIndex);i<=last;++i){
        const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
        if(!o.empty() && static_cast<int>(o.x())==x && static_cast<int>(o.y())==y) return i;
    }
    return -1;
}
int OverworldSimulation::queryCell(const PlayerSave& p,int x,int y,int startIndex) const{
    x=wrapX(x); y=wrapY(y);
    const int oi=objectAt(p,x,y,startIndex);
    if(oi>=0) return static_cast<int>(p.worldObjects[static_cast<std::size_t>(oi)].tile());
    return original_.terrainAt(x,y);
}
int OverworldSimulation::monsterCount(const PlayerSave& p) const{
    int n=0; const int last=lastObject(p);
    for(int i=1;i<=last;++i) if(isMonsterTile(p.worldObjects[static_cast<std::size_t>(i)].tile())) ++n;
    return n;
}
bool OverworldSimulation::appendObject(PlayerSave& p,const WorldObject& o) const{
    const int last=lastObject(p);
    if(last>=OriginalData::WORLD_OBJECT_COUNT-1) return false;
    p.worldObjects[static_cast<std::size_t>(last+1)]=o;
    return true;
}
void OverworldSimulation::eraseObject(PlayerSave& p,int index) const{
    const int last=lastObject(p);
    if(index<=0 || index>last) return;
    for(int i=index;i<last;++i)
        p.worldObjects[static_cast<std::size_t>(i)]=p.worldObjects[static_cast<std::size_t>(i+1)];
    p.worldObjects[static_cast<std::size_t>(last)].w.fill(0);
}

const WorldObject* OverworldSimulation::monsterAt(const PlayerSave& p,int x,int y) const{
    const int oi=objectAt(p,wrapX(x),wrapY(y),1);
    if(oi<0) return nullptr;
    const auto& o=p.worldObjects[static_cast<std::size_t>(oi)];
    return isMonsterTile(o.tile())?&o:nullptr;
}

void OverworldSimulation::maybeSpawn(PlayerSave& p){
    // Main loop 0x905E calls 0x7F0F while the tracked monster count is <=10.
    if(monsterCount(p)>10) return;

    const int levelIndex=std::min<int>(9,static_cast<int>(p.experience)/1000);
    if(sharedRng_.randomRange(1,255)>=SPAWN_THRESHOLD[static_cast<std::size_t>(levelIndex)]) return;

    // 0x7F43 consumes this random value and clamps it to the monster stat even
    // though the optimized binary never uses the result afterwards. Keep the
    // call to preserve the global DOS RNG stream exactly.
    int unused=sharedRng_.randomRange(1,8)*2;
    const int mi=sharedRng_.randomRange(0,149)/10;
    const int stat=SPAWN_STAT[static_cast<std::size_t>(mi)];
    if(unused>stat) unused=stat;
    (void)unused;

    const int terrainClass=(mi<4)?0:((mi<9)?2:1);
    const int tile=19+mi*2;
    const int level=static_cast<int>(p.experience)/1000+1; // OUT.EXE 0xB941
    const int hp=stat*10 + level + 1; // 0x7FBD..0x7FC1 performs one extra INC.

    int sx=0,sy=0,q=-1; bool coordOk=false;
    // The executable has no effective attempt counter here. It retries only if
    // both the terrain is wrong and the quadrant-seam coordinate test fails.
    // The guard is defensive against a corrupt RNG state; valid DOS data exits quickly.
    for(int guard=0;guard<4096;++guard){
        int dx=sharedRng_.randomRange(10,15);
        int dy=sharedRng_.randomRange(5,15);
        if(sharedRng_.randomRange(1,16)&1) dx=-dx;
        if(sharedRng_.randomRange(1,16)&1) dy=-dy;
        sx=wrapX(static_cast<int>(p.x)+dx);
        sy=wrapY(static_cast<int>(p.y)+dy);
        q=queryCell(p,sx,sy,1); // spawn lookup excludes player record 0
        coordOk=seamXOk(sx)&&seamYOk(sy);
        if(q!=terrainClass && !coordOk) continue;
        break;
    }
    if(q!=terrainClass || !coordOk) return;

    WorldObject o; o.w.fill(0);
    o.w[0]=static_cast<std::uint16_t>(tile);
    o.w[1]=static_cast<std::uint16_t>(terrainClass);
    o.w[2]=static_cast<std::uint16_t>(sx);
    o.w[3]=static_cast<std::uint16_t>(sy);
    o.w[4]=static_cast<std::uint16_t>(hp);
    appendObject(p,o);
}

std::uint16_t OverworldSimulation::cRandNext(){
    // OUT.EXE 0x91D4; DS:17A4 is initialized to 1 in the executable image.
    cRandState_=static_cast<std::uint16_t>((static_cast<std::uint32_t>(cRandState_)*0x0E5Du+3u)&0x7FFFu);
    return cRandState_;
}

bool OverworldSimulation::monsterMayEnter(PlayerSave& p,int objectIndex,int tx,int ty){
    const int t=std::clamp<int>(p.transport,0,7);
    if(sharedRng_.randomRange(1,100)>MOVE_CHANCE[static_cast<std::size_t>(t)]) return false;

    const int tile=static_cast<int>(p.worldObjects[static_cast<std::size_t>(objectIndex)].tile());
    int lo=1,hi=2;
    if(tile>=0x13 && tile<=0x19){ lo=hi=0; }               // four sea monsters
    else if(tile==0x1F || tile==0x23){ lo=hi=2; }         // Hidden archer / Evil trent

    tx=wrapX(tx); ty=wrapY(ty);
    const int q=queryCell(p,tx,ty,0); // movement query includes player and all objects
    const bool special=(lo!=hi && q>=4 && q<=9);
    const bool normal=((q==lo || q==hi) && seamXOk(tx) && seamYOk(ty));
    return normal || special;
}

bool OverworldSimulation::tryMonsterAxis(PlayerSave& p,int oi,bool yAxis,int dx,int dy){
    const int delta=yAxis?dy:dx;
    const int step=sign(delta);
    if(step==0) return false;
    auto& o=p.worldObjects[static_cast<std::size_t>(oi)];
    int tx=static_cast<int>(o.x()), ty=static_cast<int>(o.y());
    if(yAxis) ty=wrapY(ty+step); else tx=wrapX(tx+step);
    if(!monsterMayEnter(p,oi,tx,ty)) return false;
    o.w[2]=static_cast<std::uint16_t>(tx); o.w[3]=static_cast<std::uint16_t>(ty);
    // OUT.EXE 0x8479 refreshes word1 from the raw map terrain after moving.
    o.w[1]=static_cast<std::uint16_t>(original_.terrainAt(tx,ty));
    return true;
}

void OverworldSimulation::moveMonster(PlayerSave& p,int oi,int dx,int dy){
    // OUT.EXE 0x8598 uses a second, independent CRT-style LCG only to choose
    // which axis to try first. Odd -> Y first; even -> X first.
    if(cRandNext()&1u){
        if(!tryMonsterAxis(p,oi,true,dx,dy)) tryMonsterAxis(p,oi,false,dx,dy);
    }else{
        if(!tryMonsterAxis(p,oi,false,dx,dy)) tryMonsterAxis(p,oi,true,dx,dy);
    }
}

void OverworldSimulation::monsterAttack(PlayerSave& p,int oi,int dx,int dy,int range,
                                         std::vector<std::string>& m){
    if(oi<=0 || oi>lastObject(p)) return;
    const auto& o=p.worldObjects[static_cast<std::size_t>(oi)];
    const int tile=static_cast<int>(o.tile());
    const std::string attackerName=monsterName(tile);

    WorldEnemyAttackEvent event;
    event.attackPrefix=attackerName+"의 공격! ";

    // M140 correction of OUT.EXE 0x847C..0x8518:
    // attack eligibility and projectile direction are separate.  A monster may
    // begin a ranged attack whenever the player is inside Chebyshev range, but
    // the projectile itself has only eight possible headings.  Its heading is
    // fixed once from sign(dx),sign(dy) and NEVER bends to chase the player.
    // Thus an off-axis target such as (dx,dy)=(3,1) causes an attack attempt,
    // but the diagonal projectile travels (1,1),(2,2),(3,3) and misses.
    // As in the DOS routine, tile 50 is drawn before the terrain/player stop
    // test, so a terminal mountain or player cell remains in the visible path.
    int x=static_cast<int>(o.x()), y=static_cast<int>(o.y());
    const int sx=sign(dx), sy=sign(dy);
    bool reached=false;
    for(int step=1;step<=range;++step){
        x=wrapX(x+sx);
        y=wrapY(y+sy);

        event.path.push_back({x,y});

        const int terrain=original_.terrainAt(x,y);
        if(terrain==3) break;
        if(x==static_cast<int>(p.x) && y==static_cast<int>(p.y)){
            reached=true;
            break;
        }
    }

    const int roll=sharedRng_.randomRange(1,255); // OUT.EXE 0x8518, even if blocked
    const int threshold=200-static_cast<int>(p.stamina)/2-static_cast<int>(p.readyArmour)*8;

    if(!reached || roll>=threshold){
        event.resultSuffix="빗맞음!";
        m.push_back(event.attackPrefix+event.resultSuffix);
        enemyAttackEvents_.push_back(std::move(event));
        return;
    }

    const int damage=sharedRng_.randomRange(1,monsterPower(tile)*2+1);
    event.hit=true;
    event.damage=damage;
    event.resultSuffix="적중! "+std::to_string(damage)+"의 피해량!";
    m.push_back(event.attackPrefix+event.resultSuffix);
    enemyAttackEvents_.push_back(std::move(event));

    p.hits=static_cast<std::uint16_t>(
        damage>=static_cast<int>(p.hits)?0:static_cast<int>(p.hits)-damage);
}

std::vector<std::string> OverworldSimulation::updateAfterCommand(PlayerSave& p){
    enemyAttackEvents_.clear();
    std::vector<std::string> out;
    std::vector<int> queued;
    const int last=lastObject(p);
    for(int i=1;i<=last;++i){
        const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
        const int tile=static_cast<int>(o.tile());
        if(!isMonsterTile(tile)) continue;
        const int dx=static_cast<int>(p.x)-static_cast<int>(o.x());
        const int dy=static_cast<int>(p.y)-static_cast<int>(o.y());
        const int range=monsterRange(tile);
        if(std::max(std::abs(dx),std::abs(dy))<=range) queued.push_back(i);
        else moveMonster(p,i,dx,dy);
    }

    // 0x8686 consumes the queued list in reverse insertion order.
    for(auto it=queued.rbegin();it!=queued.rend() && p.hits>0;++it){
        const int i=*it;
        if(i>lastObject(p)) continue;
        const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
        if(!isMonsterTile(o.tile())) continue;
        const int dx=static_cast<int>(p.x)-static_cast<int>(o.x());
        const int dy=static_cast<int>(p.y)-static_cast<int>(o.y());
        const int range=monsterRange(o.tile());
        const int inRange=(std::max(std::abs(dx),std::abs(dy))<=range)?range:0;
        if(inRange) monsterAttack(p,i,dx,dy,inRange,out);
    }
    return out;
}

std::vector<WorldEnemyAttackEvent> OverworldSimulation::takeEnemyAttackEvents(){
    auto out=std::move(enemyAttackEvents_);
    enemyAttackEvents_.clear();
    return out;
}

void OverworldSimulation::cleanupOldQuadrant(PlayerSave& p,int oldQuadrant){
    // OUT.EXE 0x86E1 removes monsters whose quadrant equals the quadrant the
    // player just left; transports and other objects are retained.
    for(int i=1;i<=lastObject(p);++i){
        const auto& o=p.worldObjects[static_cast<std::size_t>(i)];
        if(isMonsterTile(o.tile()) && quadrant(o.x(),o.y())==oldQuadrant){ eraseObject(p,i); --i; }
    }
}

void OverworldSimulation::onPlayerMoved(PlayerSave& p,int oldX,int oldY){
    const int oldQ=quadrant(oldX,oldY), newQ=quadrant(p.x,p.y);
    if(oldQ!=newQ) cleanupOldQuadrant(p,oldQ);
}

std::string OverworldSimulation::addCoinReward(PlayerSave& p,int amount) const{
    // OUT.EXE 0x0761 caps Coin at 9999 and emits the denominations in order.
    // M115 keeps them in one logical text stream so denomination boundaries
    // never force command-window line breaks.
    const int room=9999-static_cast<int>(p.coin);
    const int add=std::max(0,std::min(amount,room));
    if(add<=0) return {};
    p.coin=static_cast<std::uint16_t>(static_cast<int>(p.coin)+add);

    const int copper=add%10;
    const int silver=(add%100-copper)/10;
    const int gold=add/1000;

    std::string reward;
    const auto append=[&](const std::string& part){
        if(!reward.empty()) reward+=" ";
        reward+=part;
    };
    if(copper) append("동화 "+std::to_string(copper)+"냥!");
    if(silver) append("은화 "+std::to_string(silver)+"냥!");
    if(gold) append("금화 "+std::to_string(gold)+"냥!");
    return reward;
}

std::vector<std::string> OverworldSimulation::attack(PlayerSave& p,int dx,int dy,WorldAttackMode mode){
    std::vector<std::string> out;
    lastPlayerAttackHit_=false;
    int range=0, hitChance=0, damage=0;
    if(mode==WorldAttackMode::Normal){
        const int weapon=std::clamp<int>(p.readyWeapon,0,15);
        range=OriginalData::weaponRange(weapon);
        hitChance=static_cast<int>(p.agility)+50;
        damage=sharedRng_.randomRange(2,static_cast<int>(p.strength)+weapon*8);
    }else if(mode==WorldAttackMode::Fire){
        range=3; hitChance=80;
        damage=sharedRng_.randomRange(1,static_cast<int>(p.transport)*10)+30;
    }else if(mode==WorldAttackMode::MagicMissile){
        // OUT.EXE spell success has already been resolved by the Cast command.
        // The projectile then uses a fixed three-cell world ray; no second
        // weapon/agility hit roll is applied to a successfully cast spell.
        range=3; hitChance=100;
        damage=sharedRng_.randomRange(1,std::max(1,static_cast<int>(p.intelligence)));
        if(p.readyWeapon==9) damage*=2;
        else if(p.readyWeapon==8) damage=(damage*3)/2;
        else if(p.readyWeapon==10 || p.readyWeapon==11) damage*=3;
    }else{ // KillSpell
        range=3; hitChance=100; damage=9999;
    }
    if(range<=0){ out.push_back("공격이 빗나갑니다!"); return out; }

    int tx=p.x,ty=p.y,q=-1;
    for(int dist=1;dist<=range;++dist){
        tx=wrapX(static_cast<int>(p.x)+dx*dist);
        ty=wrapY(static_cast<int>(p.y)+dy*dist);
        q=queryCell(p,tx,ty,1); // common scan excludes player record 0
        if(q>=19 || q==3) break;
    }

    // Missed! here means that the attack travelled into empty space / no target.
    if(q<19 || !isMonsterTile(q)){ out.push_back("공격이 빗나갑니다!"); return out; }
    const int oi=objectAt(p,tx,ty,1);
    if(oi<0){ out.push_back("공격이 빗나갑니다!"); return out; }

    const std::string name=monsterName(q);
    const std::string targetObject=조사_처리기("["+name+"]@을");

    // Missed the [target]! is distinct from empty-space Missed!.
    if(sharedRng_.randomRange(1,100)>hitChance){
        out.push_back(targetObject+" 빗맞힘!");
        return out;
    }

    auto& target=p.worldObjects[static_cast<std::size_t>(oi)];
    lastPlayerAttackHit_=true;

    // Hit/damage are the first fragments of one continuous logical stream.
    std::string result=name+"에게 적중! "+std::to_string(damage)+"의 피해량!";

    if(damage<static_cast<int>(target.w[4])){
        target.w[4]=static_cast<std::uint16_t>(static_cast<int>(target.w[4])-damage);
        out.push_back(result);
        return out;
    }

    // M119 presentation: a lethal hit finishes the hit/damage row first.
    // Kill + loot start on the next logical command-window row, then remain
    // continuous there until the width-aware wrapper has to wrap visually.
    out.push_back(result);

    std::string killReward=targetObject+" 처치했습니다!";
    const int power=monsterPower(q);
    eraseObject(p,oi);
    const int coinReward=sharedRng_.randomRange(1,power*4)+10;
    const int expReward=sharedRng_.randomRange(2,power*5);
    p.experience=static_cast<std::uint16_t>(
        std::min(9999,static_cast<int>(p.experience)+expReward));

    const std::string reward=addCoinReward(p,coinReward);
    if(!reward.empty()) killReward+=" "+reward;
    out.push_back(killReward);
    return out;
}