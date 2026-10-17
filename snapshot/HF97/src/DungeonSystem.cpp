#include "DungeonSystem.h"
#include "KoreanText.h"
#include <algorithm>
#include <chrono>
#include <cctype>
#include <cmath>
#include <ctime>

namespace {
constexpr std::array<int,6> PX={0,72,108,126,135,144};
constexpr std::array<int,6> PY={0,36,54,63,68,72};
constexpr std::array<int,16> EXTRA_X={4,8,5,2,8,1,5,4,6,1,3,7,3,9,2,6};
constexpr std::array<int,16> EXTRA_Y={1,1,2,3,3,4,4,5,5,6,6,6,8,8,9,9};
constexpr int ORIG_LEFT=16, ORIG_RIGHT=303, ORIG_TOP=8, ORIG_BOTTOM=151;
inline int sx(int x){ return x*2; }
inline int sy(int y){ return y*2; }

std::string dungeonTreasureText(int amount){
    // OUT.EXE 0x0761 denomination display, localized per the approved dungeon text.
    // Zero-valued denominations are omitted, matching the original helper.
    const int copper=amount%10;
    const int silver=(amount%100-copper)/10;
    const int gold=amount/1000;
    std::string out;
    const auto append=[&](const std::string& part){
        if(!out.empty()) out+=" ";
        out+=part;
    };
    if(copper) append("동화 "+std::to_string(copper)+"냥!");
    if(silver) append("은화 "+std::to_string(silver)+"냥!");
    if(gold) append("금화 "+std::to_string(gold)+"냥!");
    return out;
}

// The DOS renderer draws in a 320x200 logical raster.  Our presentation is
// 640x400, so every original pixel is emitted as a 2x2 block.  The stepping
// below mirrors OUT.EXE raw 0xAAAC rather than relying on host SDL line
// rasterization, which can differ by one pixel on diagonal tie cases.
void plotLogical(SDL_Renderer* r,int x,int y){
    SDL_Rect q{sx(x),sy(y),2,2};
    SDL_RenderFillRect(r,&q);
}
void line(SDL_Renderer* r,int x0,int y0,int x1,int y1){
    int xstep=1, ystep=1;
    int dy=y1-y0; if(dy<0){ystep=-1;dy=-dy;}
    int dx=x1-x0; if(dx<0){xstep=-1;dx=-dx;}
    bool yMajor=false;
    int major=dx, minor=dy;
    if(dx<dy){yMajor=true;major=dy;minor=dx;}
    int err=2*minor-major;
    const int errStraight=2*minor;
    const int errDiag=2*minor-2*major;
    int x=x0,y=y0;
    for(int n=0;n<=major;++n){
        plotLogical(r,x,y);
        if(err<0){
            if(yMajor) y+=ystep; else x+=xstep;
            err+=errStraight;
        }else{
            x+=xstep; y+=ystep; err+=errDiag;
        }
    }
}
void rect4(SDL_Renderer* r,int l,int t,int rr,int b){
    line(r,l,t,rr,t); line(r,rr,t,rr,b); line(r,rr,b,l,b); line(r,l,b,l,t);
}

int dosVectorScale(std::int8_t v,int shift){
    // OUT.EXE 0xAF06: CBW, logical SHR CL, then CBW again.  Reproduce that
    // sequence exactly instead of using implementation-defined signed shifts.
    std::uint16_t ax=static_cast<std::uint16_t>(static_cast<std::int16_t>(v));
    if(shift>0) ax=static_cast<std::uint16_t>(ax>>shift);
    return static_cast<int>(static_cast<std::int8_t>(ax&0xFFu));
}
}

DungeonCell& DungeonSystem::cell(DungeonState& d,int x,int y) const{
    return d.cells[static_cast<std::size_t>(y*11+x)];
}
const DungeonCell& DungeonSystem::cell(const DungeonState& d,int x,int y) const{
    static const DungeonCell wall{1,0xFFFF,0,0};
    if(x<0||x>10||y<0||y>10) return wall;
    return d.cells[static_cast<std::size_t>(y*11+x)];
}

std::uint16_t DungeonSystem::dosTimeHash(){
    // OUT.EXE file 0xAF24, DOS int21/AH=2Ch. Same hash used by the game's RNG seed path.
#ifdef U1_CAMPAIGN_DUNGEON_TIME_HASH
    // Headless campaign regression only: CMake compiles DungeonSystem.cpp for the
    // smoke target with a fixed, valid DOS time-hash so CI is reproducible.
    return static_cast<std::uint16_t>(U1_CAMPAIGN_DUNGEON_TIME_HASH);
#endif
    using namespace std::chrono;
    const auto now=system_clock::now();
    const auto tt=system_clock::to_time_t(now);
    std::tm tm{};
#if defined(__DJGPP__)
    if(const std::tm* dosTm=std::localtime(&tt)) tm=*dosTm;
#elif defined(_WIN32)
    localtime_s(&tm,&tt);
#else
    localtime_r(&tt,&tm);
#endif
    const auto ms=duration_cast<milliseconds>(now.time_since_epoch()).count();
    const int hundredths=static_cast<int>((ms%1000)/10);
    const std::uint16_t cx=static_cast<std::uint16_t>((((tm.tm_hour*2)&0xFF)<<8)|((tm.tm_min*4)&0xFF));
    const std::uint16_t dx=static_cast<std::uint16_t>((((tm.tm_sec*8)&0xFF)<<8)|(hundredths&0xFF));
    return static_cast<std::uint16_t>((static_cast<std::uint16_t>(cx+dx)^0x91EBu)&0x0FFFu);
}

void DungeonSystem::enter(DungeonState& d,const PlayerSave& p,int locationId,std::vector<std::string>& messages){
    d=DungeonState{};
    d.active=true; d.locationId=locationId; d.x=1; d.y=1; d.level=1; d.orientation=4; d.exitHitBonus=0;
    messages.push_back("Entering...");
    generate(d,p);
}

void DungeonSystem::regenerate(DungeonState& d,const PlayerSave& p){ generate(d,p); }

void DungeonSystem::generate(DungeonState& d,const PlayerSave& p){
    // OUT.EXE file 0x4961. First seed is deterministic from world position,
    // dungeon level and PLAYER.U1 +0x32; later content is re-seeded from DOS time+1777.
    const std::uint16_t deterministic=static_cast<std::uint16_t>(
        static_cast<int>(p.x)*5 + static_cast<int>(p.y)*3 + d.level*17 + static_cast<int>(p.rngSeedWord));
    rng_.seed(deterministic);

    for(auto& c:d.cells){ c.feature=0; c.monster=0xFFFF; c.marker=0; c.hp=0; }
    // Border walls.
    for(int i=0;i<=10;++i){ cell(d,i,0).feature=1; cell(d,i,10).feature=1; cell(d,0,i).feature=1; cell(d,10,i).feature=1; }

    // OUT.EXE load-image 0x47F9..0x4872 addresses cells as x*0x58 + y*8.
    // Therefore x=2,4,6,8 are the columns being processed (not rows). The
    // first even/even wall assignment is then overwritten by the executable's
    // random 0..3 pass for y=1..9; we preserve that seemingly redundant order.
    for(int x=2;x<=8;x+=2){
        for(int y=2;y<=8;y+=2) cell(d,x,y).feature=1;
    }
    for(int x=2;x<=8;x+=2){
        for(int y=1;y<=9;++y) cell(d,x,y).feature=static_cast<std::uint16_t>(rng_.randomRange(1,4)-1);
    }

    // Extra level-scaled structures: loop is inclusive, therefore 2*level+1 placements.
    for(int i=0;i<=d.level*2;++i){
        const int feature=(rng_.randomRange(1,255)>160)?8:1;
        const int k=rng_.randomRange(1,16)-1;
        int x,y;
        if(d.level&1){ x=EXTRA_X[static_cast<std::size_t>(k)]; y=EXTRA_Y[static_cast<std::size_t>(k)]; }
        else { x=EXTRA_Y[static_cast<std::size_t>(k)]; y=EXTRA_X[static_cast<std::size_t>(k)]; }
        cell(d,x,y).feature=static_cast<std::uint16_t>(feature);
    }

    // Dynamic contents use a new time seed in the original.
    rng_.seed(static_cast<std::uint16_t>(dosTimeHash()+0x06F1u));
    for(int i=0;i<=d.level;++i){
        const int marker=(rng_.randomRange(1,100)&1)+4; // 4 chest / 5 coffin
        const int x=rng_.randomRange(10,99)/10;
        const int y=rng_.randomRange(10,99)/10;
        auto& c=cell(d,x,y);
        if(c.feature!=1 && c.feature!=2 && c.feature!=8) c.marker=static_cast<std::uint16_t>(marker);
    }

    // Alternating paired ladders.
    if(d.level&1){ cell(d,7,3).feature=7; cell(d,3,7).feature=6; }
    else { cell(d,7,3).feature=6; cell(d,3,7).feature=7; }
    if(d.level==10) cell(d,7,3).feature=0;
    if(d.level==1){ cell(d,1,1).feature=7; cell(d,7,3).feature=0; }

    // 0x4802 is called exactly three times after clearing five uniqueness flags.
    for(int i=0;i<3;++i) placeMonster(d);
}

void DungeonSystem::placeMonster(DungeonState& d){
    // The original keeps five selection flags per generator call batch. We need the
    // flags to persist across the three calls; encode them by scanning existing IDs
    // in this level's five-monster band.
    std::array<bool,5> used{};
    const int band=((d.level-1)/2)*5;
    for(const auto& c:d.cells){
        if(c.monster!=0xFFFF && c.monster>=band && c.monster<band+5)
            used[static_cast<std::size_t>(c.monster-band)]=true;
    }

    int x=1,y=1; bool valid=false;
    for(int tries=0;tries<=500;++tries){
        x=(rng_.randomRange(0,242)%9)+1;
        y=(rng_.randomRange(0,242)%9)+1;
        auto& c=cell(d,x,y);
        valid=(c.monster==0xFFFF && c.feature!=1 && c.feature!=2 && c.feature!=8 && c.marker!=4 && !(x==d.x&&y==d.y));
        if(valid) break;
    }
    if(!valid) return;

    int choice=0;
    for(int guard=0;guard<256;++guard){
        choice=rng_.randomRange(0,49)/10;
        choice=std::clamp(choice,0,4);
        if(!used[static_cast<std::size_t>(choice)]) break;
    }
    const int id=band+choice;
    auto& c=cell(d,x,y);
    c.monster=static_cast<std::uint16_t>(id);
    c.hp=static_cast<std::uint16_t>(rng_.randomRange(1,d.level*d.level+1)+10+id);
}

bool DungeonSystem::spawnMonsterAt(DungeonState& d,int x,int y){
    // OUT.EXE 0x5AF8: coffin trap monster placement at a caller-selected cell.
    auto& c=cell(d,x,y);
    if(!(c.feature==0 || c.feature==6 || c.feature==7) || c.monster!=0xFFFFu) return false;
    std::array<bool,5> used{};
    const int band=((d.level-1)/2)*5;
    for(const auto& q:d.cells) if(q.monster!=0xFFFFu && q.monster>=band && q.monster<band+5)
        used[static_cast<std::size_t>(q.monster-band)]=true;
    int choice=0; bool found=false;
    for(int tries=0;tries<50;++tries){
        choice=rng_.randomRange(0,49)/10;
        choice=std::clamp(choice,0,4);
        if(!used[static_cast<std::size_t>(choice)]){found=true;break;}
    }
    if(!found) return false;
    const int id=band+choice;
    c.monster=static_cast<std::uint16_t>(id);
    c.hp=static_cast<std::uint16_t>(rng_.randomRange(1,d.level*d.level+1)+10+id);
    return true;
}

void DungeonSystem::delta(int o,int& dx,int& dy) const{
    dx=dy=0;
    if(o==1) dx=-1; else if(o==2) dx=1; else if(o==3) dy=-1; else if(o==4) dy=1;
}
void DungeonSystem::leftDelta(int o,int& dx,int& dy) const{
    int fx,fy; delta(o,fx,fy); dx=fy; dy=-fx;
}
const char* DungeonSystem::orientationName(int o){
    switch(o){ case 1:return "West"; case 2:return "East"; case 3:return "North"; case 4:return "South"; default:return ""; }
}

void DungeonSystem::turnAround(DungeonState& d,std::vector<std::string>& m) const{
    m.push_back("뒤로 돌기");
    static constexpr int map[5]={0,2,1,4,3}; d.orientation=map[std::clamp(d.orientation,1,4)];
}
void DungeonSystem::turnLeft(DungeonState& d,std::vector<std::string>& m) const{
    m.push_back("좌측 회전");
    static constexpr int map[5]={0,4,3,1,2}; // exact OUT jump table
    d.orientation=map[std::clamp(d.orientation,1,4)];
    // Note: the turn-around table happens to have the same numeric mapping only for
    // W/E/N/S encoding? Correct left transitions are W->S,E->N,N->W,S->E = 1->4,2->3,3->1,4->2.
}
void DungeonSystem::turnRight(DungeonState& d,std::vector<std::string>& m) const{
    m.push_back("우측 회전");
    static constexpr int map[5]={0,3,4,2,1}; d.orientation=map[std::clamp(d.orientation,1,4)];
}

bool DungeonSystem::blocksMove(const DungeonCell& from,const DungeonCell& to){
    if(to.feature==1 || to.feature==2 || to.feature==8) return true;
    if(to.feature==3 && from.feature==3) return true;
    if(to.monster!=0xFFFF) return true;
    return false;
}

void DungeonSystem::forward(DungeonState& d,std::vector<std::string>& m) const{
    int dx,dy; delta(d.orientation,dx,dy);
    m.push_back("전진");
    const auto& from=cell(d,d.x,d.y); const auto& to=cell(d,d.x+dx,d.y+dy);
    if(blocksMove(from,to)){ m.back()+=" - 길이 막혔습니다!"; return; }
    d.x+=dx; d.y+=dy;
}

void DungeonSystem::searchSecret(DungeonState& d,std::vector<std::string>& m) const{
    int dx,dy; delta(d.orientation,dx,dy);
    auto& target=cell(d,d.x+dx,d.y+dy);
    if(target.feature==2 && cell(d,d.x,d.y).feature!=3){
        m.push_back("비밀문입니다!"); target.feature=3;
    }else m.push_back("아무것도 없습니다.");
}

void DungeonSystem::openCoffin(DungeonState& d,PlayerSave& p,std::vector<std::string>& m){
    m.push_back("관 열기");
    auto& here=cell(d,d.x,d.y);
    if(here.marker!=5){m.back()+=" - 관이 없습니다!";return;}
    int dx,dy; delta(d.orientation,dx,dy);
    const int tx=d.x+dx, ty=d.y+dy;
    const int roll=rng_.randomRange(1,255);
    // The coffin monster branch has no extra trap text in OUT.EXE: the spawned
    // monster itself is the result after the completed Open coffin command row.
    if(roll<104 && spawnMonsterAt(d,tx,ty)) return;
    here.marker=0;
    const int reward=rng_.randomRange(3,d.level*d.level+9);
    const int room=9999-static_cast<int>(p.coin);
    const int actual=std::max(0,std::min(room,reward));
    p.coin=static_cast<std::uint16_t>(static_cast<int>(p.coin)+actual);
    const std::string found=dungeonTreasureText(actual);
    if(!found.empty()) m.push_back(found);
}

void DungeonSystem::unlockChest(DungeonState& d,PlayerSave& p,std::vector<std::string>& m){
    m.push_back("상자 열기");
    auto& here=cell(d,d.x,d.y);
    if(here.marker!=4){m.back()+=" - 상자가 없습니다!";return;}
    const int roll=rng_.randomRange(1,75);
    if(roll>static_cast<int>(p.agility) && p.klass!=3){
        m.push_back("함정이 발동하였습니다!");
        p.hits=static_cast<std::uint16_t>(d.level>=static_cast<int>(p.hits)?0:static_cast<int>(p.hits)-d.level);
        return;
    }
    here.marker=0;
    const int reward=rng_.randomRange(3,d.level*d.level+9);
    const int room=9999-static_cast<int>(p.coin);
    const int actual=std::max(0,std::min(room,reward));
    p.coin=static_cast<std::uint16_t>(static_cast<int>(p.coin)+actual);
    const std::string found=dungeonTreasureText(actual);
    if(!found.empty()) m.push_back(found);
}

bool DungeonSystem::climb(DungeonState& d,PlayerSave& p,std::vector<std::string>& m){
    const int f=cell(d,d.x,d.y).feature;
    if(f!=6 && f!=7){ m.push_back("무슨 사다리를 탑니까?"); return false; }
    if(d.orientation==1 || d.orientation==2){
        m.push_back("사다리 타기");
        m.push_back("사다리를 정면으로 보아야 합니다!");
        return false;
    }
    if(f==7){
        --d.level;
        if(d.level==0) m.push_back("사다리를 타고 지상으로 올라갑니다");
        else m.push_back("사다리를 타고 "+std::to_string(d.level)+"층으로 올라갑니다");
        if(d.level==0){
            if(d.exitHitBonus>0){
                const int room=9999-static_cast<int>(p.hits);
                const int gain=std::max(0,std::min(room,d.exitHitBonus));
                p.hits=static_cast<std::uint16_t>(static_cast<int>(p.hits)+gain);
                if(gain>0) m.push_back("생명력이 "+std::to_string(gain)+"만큼 올랐습니다.");
            }
            d.active=false; return true;
        }
        generate(d,p);
        cell(d,d.x,d.y).monster=0xFFFF; // exact post-regeneration clear in up routine
        return false;
    }
    ++d.level;
    m.push_back("사다리를 타고 "+std::to_string(d.level)+"층으로 내려갑니다");
    generate(d,p);
    return false;
}


const char* DungeonSystem::monsterName(int id){
    static constexpr const char* NAMES[25]={
        "레인저","해골","도적","거대 시궁쥐","박쥐",
        "거미","독사","오크","키클롭스","젤라틴 큐브",
        "에틴","상자","리자드맨","미노타우루스","송장 벌레",
        "죽음의 신","그렘린","떠도는 눈","망령","리치",
        "투명 수색꾼","정신 타격자","조른","악마","발론"
    };
    return (id>=0 && id<25)?NAMES[id]:"Monster";
}

bool DungeonSystem::completeMonsterQuest(int monsterId,PlayerSave& p,std::vector<std::string>& messages) const{
    // OUT.EXE file 0xAEE (load-image 0x8EE): exact switch keys are
    // 24,19,14,9 and map to king quest slots 8,6,4,2 respectively.
    int castle=0;
    if(monsterId==9) castle=2;
    else if(monsterId==14) castle=4;
    else if(monsterId==19) castle=6;
    else if(monsterId==24) castle=8;
    if(!castle) return false;
    auto& q=p.kingQuest[static_cast<std::size_t>(castle)];
    if(q!=0xFFFFu) return false;
    q=1;
    messages.push_back("임무를 완료하였습니다!");
    return true;
}

void DungeonSystem::attack(DungeonState& d,PlayerSave& p,std::vector<std::string>& messages){
    const int weapon=std::clamp<int>(p.readyWeapon,0,15);
    const int range=OriginalData::weaponRange(weapon);
    if(range<=0){ messages.push_back("대상이 없습니다"); return; }

    int dx,dy; delta(d.orientation,dx,dy);
    int tx=-1,ty=-1;
    for(int step=1;step<=range;++step){
        const int x=d.x+dx*step, y=d.y+dy*step;
        const auto& c=cell(d,x,y);
        if(c.monster!=0xFFFF){ tx=x;ty=y;break; }
        if(c.feature==1 || c.feature==2 || c.feature==8) break;
    }
    if(tx<0){ messages.push_back("대상이 없습니다"); return; }

    auto& target=cell(d,tx,ty);
    const int hitChance=static_cast<int>(p.agility)+50;
    if(rng_.randomRange(1,100)>hitChance){ messages.push_back("빗맞음!"); return; }

    const int upper=static_cast<int>(p.strength)+weapon*8;
    const int damage=rng_.randomRange(2,std::max(2,upper));
    const int id=static_cast<int>(target.monster);
    messages.push_back("적중! "+std::to_string(damage)+"의 피해량!");
    if(static_cast<int>(target.hp)>damage){
        target.hp=static_cast<std::uint16_t>(static_cast<int>(target.hp)-damage);
        return;
    }

    // OUT.EXE 0xCAC..0xD57: reward and experience use separate random calls.
    const int reward=rng_.randomRange(2,d.level*3+10+id);
    const int room=9999-static_cast<int>(p.coin);
    const int actual=std::max(0,std::min(room,reward));
    p.coin=static_cast<std::uint16_t>(static_cast<int>(p.coin)+actual);

    const int copper=actual%10;
    const int silver=(actual%100)/10;
    const int gold=actual/100;
    std::string kill=std::string(monsterName(id))+" 처치!";
    if(copper) kill+=" 동화 "+std::to_string(copper)+"냥!";
    if(silver) kill+=" 은화 "+std::to_string(silver)+"냥!";
    if(gold) kill+=" 금화 "+std::to_string(gold)+"냥!";
    messages.push_back(kill);
    completeMonsterQuest(id,p,messages);

    target.monster=0xFFFF; target.hp=0;
    const int expGain=rng_.randomRange(2,d.level*id+10);
    p.experience=static_cast<std::uint16_t>(std::min(9999,static_cast<int>(p.experience)+expGain));
    d.exitHitBonus+=expGain*2;

    // OUT.EXE immediately calls the level monster placement routine again.
    placeMonster(d);
}

bool DungeonSystem::moveMonster(DungeonState& d,int x,int y,int dx,int dy){
    if(dx==0 && dy==0) return false;
    const int nx=x+dx, ny=y+dy;
    if(nx<1||nx>9||ny<1||ny>9) return false;
    auto& src=cell(d,x,y); auto& dst=cell(d,nx,ny);
    if(dst.feature==1 || dst.feature==2 || dst.feature==8 || dst.monster!=0xFFFF) return false;
    dst.monster=src.monster; dst.hp=src.hp;
    src.monster=0xFFFF; src.hp=0;
    // OUT.EXE +0x3E8 temporary marker prevents a moved monster from moving twice
    // while the x-major 1..9 scan is still in progress.
    dst.marker=static_cast<std::uint16_t>(dst.marker+1000u);
    return true;
}

void DungeonSystem::monsterAttack(DungeonState& d,PlayerSave& p,int id,std::vector<std::string>& messages){
    const std::string attacker=std::string(monsterName(id));
    const int roll=rng_.randomRange(1,255);
    const int evade=static_cast<int>(p.stamina)/2 + static_cast<int>(p.readyArmour)*8 + 56;
    if(roll<=evade){ messages.push_back(attacker+"의 공격! 빗맞음!"); return; }

    // OUT.EXE 0x8A81..0x8BB8: four monster-specific effects.
    if(id==9 && p.readyArmour!=0){
        messages.push_back(attacker+"의 공격! 방어구가 파괴되었습니다!");
        const int a=std::clamp<int>(p.readyArmour,0,static_cast<int>(p.armour.size())-1);
        if(a>0 && p.armour[static_cast<std::size_t>(a)]>0) --p.armour[static_cast<std::size_t>(a)];
        p.readyArmour=0;
        return;
    }
    if(id==16){
        messages.push_back(attacker+"의 공격! 그렘린이 식량을 훔쳤습니다!");
        p.food=static_cast<std::uint16_t>(p.food/2);
        return;
    }
    if(id==21 && roll<128){
        messages.push_back(attacker+"의 공격! 정신 공격!");
        p.intelligence=static_cast<std::uint16_t>(p.intelligence/2 + 5);
        return;
    }
    std::string thiefEvent;
    if(id==2){
        for(int i=1;i<16;++i){
            if(i==static_cast<int>(p.readyWeapon) || p.weapons[static_cast<std::size_t>(i)]==0) continue;
            thiefEvent=조사_처리기("도적이 ["+std::string(무기_이름_한글(i))+"]@을 훔쳤습니다!");
            --p.weapons[static_cast<std::size_t>(i)];
            break;
        }
        // The executable still reaches normal damage after a theft. Keep the
        // entire enemy attack in one logical stream: attack prefix -> theft
        // event (when any) -> hit/damage. Only renderer auto-wrap may break it.
    }

    int damage=id*id+d.level;
    if(damage>255) damage=rng_.randomRange(id+1,255);
    std::string attackLine=attacker+"의 공격! ";
    if(!thiefEvent.empty()) attackLine+=thiefEvent+" ";
    attackLine+="적중! "+std::to_string(damage)+"의 피해량!";
    messages.push_back(attackLine);
    p.hits=static_cast<std::uint16_t>(damage>=static_cast<int>(p.hits)?0:static_cast<int>(p.hits)-damage);
}



void DungeonSystem::castDungeon(DungeonState& d,PlayerSave& p,int spell,std::vector<std::string>& messages){
    // OUT.EXE raw 0x15A8..0x1968.  Dungeon spells do not use the outdoor
    // Wisdom gate and do not ask for a fresh direction: directional effects
    // use the dungeon orientation already stored in the live state.
    int sp=std::clamp(spell,0,10);
    lastCastEffectiveSpell_=sp;
    lastCastFailed_=false;
    lastCastHit_=false;
    if(sp==0){
        // Prayer (raw 0x1969): choose 1..10, with the unused Steal slot 4
        // redirected to Ladder Down (6).  Prayer itself is not consumed.
        sp=rng_.randomRange(1,10);
        if(sp==4) sp=6;
    }
    lastCastEffectiveSpell_=sp;

    const auto fail=[&](){ lastCastFailed_=true; messages.push_back("주문 실패!"); };
    const auto treasure=[&](){
        const int reward=rng_.randomRange(3,d.level*d.level+9);
        const int room=9999-static_cast<int>(p.coin);
        const int actual=std::max(0,std::min(room,reward));
        p.coin=static_cast<std::uint16_t>(static_cast<int>(p.coin)+actual);
        const std::string found=dungeonTreasureText(actual);
        if(!found.empty()) messages.push_back(found);
    };
    const auto finishMonster=[&](DungeonCell& target,int id,const std::string& deathText){
        lastCastHit_=true;
        messages.push_back(std::string(monsterName(id))+deathText);
        const std::size_t deathRowIndex=messages.size()-1;
        completeMonsterQuest(id,p,messages);

        const int reward=rng_.randomRange(2,d.level*3+10+id);
        const int room=9999-static_cast<int>(p.coin);
        const int actual=std::max(0,std::min(room,reward));
        p.coin=static_cast<std::uint16_t>(static_cast<int>(p.coin)+actual);
        const std::string found=dungeonTreasureText(actual);
        if(!found.empty()) messages[deathRowIndex]+=" "+found;

        target.monster=0xFFFFu;
        target.hp=0;
        const int expGain=rng_.randomRange(2,d.level*id+10);
        p.experience=static_cast<std::uint16_t>(std::min(9999,static_cast<int>(p.experience)+expGain));
        d.exitHitBonus+=expGain*2;
        placeMonster(d);
    };

    switch(sp){
    case 1: { // Open / APERTUS
        auto& here=cell(d,d.x,d.y);
        if(here.marker!=5){ fail(); break; }
        here.marker=0;
        messages.push_back("관이 열렸습니다!");
        treasure();
        break;
    }
    case 2: { // Unlock / PECUNIA
        auto& here=cell(d,d.x,d.y);
        if(here.marker!=4){ fail(); break; }
        here.marker=0;
        messages.push_back("상자가 열렸습니다!");
        treasure();
        break;
    }
    case 3: { // Magic Missile / VASTO -- current facing, max five cells
        int dx=0,dy=0; delta(d.orientation,dx,dy);
        int tx=-1,ty=-1;
        for(int step=1;step<=5;++step){
            const int x=d.x+dx*step, y=d.y+dy*step;
            const auto& c=cell(d,x,y);
            // raw 0x0DAA stops on a monster or on feature 1/2/3/8.
            if(c.monster!=0xFFFFu){
                if(c.feature!=1 && c.feature!=2 && c.feature!=8){ tx=x; ty=y; }
                break;
            }
            if(c.feature==1 || c.feature==2 || c.feature==3 || c.feature==8) break;
        }
        if(tx<0){ fail(); break; }
        auto& target=cell(d,tx,ty);
        lastCastHit_=true;
        int damage=rng_.randomRange(1,std::max(1,static_cast<int>(p.intelligence)));
        // OUT.EXE raw 0x14F2 ready-weapon amplifiers.
        if(p.readyWeapon==9) damage*=2;
        else if(p.readyWeapon==8) damage=(damage*3)/2;
        else if(p.readyWeapon==10 || p.readyWeapon==11) damage*=3;
        const int id=static_cast<int>(target.monster);
        if(static_cast<int>(target.hp)>damage){
            target.hp=static_cast<std::uint16_t>(static_cast<int>(target.hp)-damage);
            messages.push_back("적중! "+std::to_string(damage)+"의 피해량!");
        }else finishMonster(target,id," 처치!");
        break;
    }
    case 4:
        // NUDO! is a deliberately empty slot in raw 0x15A8.  The incantation
        // is still printed by Cast, but there is no result message/effect.
        break;
    case 5: { // Ladder Up / ASCENDO
        auto& here=cell(d,d.x,d.y);
        if(here.feature==8 || ((d.x&1)==0 && (d.y&1)==0)){
            fail(); break;
        }
        here.feature=7;
        messages.push_back("사다리가 생성되었습니다!");
        break;
    }
    case 6: { // Ladder Down / INFERUS
        auto& here=cell(d,d.x,d.y);
        if(d.level==10 || here.feature==8 || ((d.x&1)==0 && (d.y&1)==0)){
            fail(); break;
        }
        here.feature=6;
        messages.push_back("사다리가 생성되었습니다!");
        break;
    }
    case 7: { // Blink / DUCIS-EDUCO
        // raw 0x177D chooses random interior coordinates until it finds a cell
        // that is not the current cell, not feature 1/2/8, and has no monster.
        for(;;){
            const int nx=rng_.randomRange(1,9);
            const int ny=rng_.randomRange(1,9);
            if(nx==d.x && ny==d.y) continue;
            const auto& c=cell(d,nx,ny);
            if(c.feature==1 || c.feature==2 || c.feature==8 || c.monster!=0xFFFFu) continue;
            d.x=nx; d.y=ny;
            messages.push_back("순간이동!");
            break;
        }
        break;
    }
    case 8: { // Create / STRUXI -- one cell in front
        int dx=0,dy=0; delta(d.orientation,dx,dy);
        auto& target=cell(d,d.x+dx,d.y+dy);
        if(target.feature!=0 || target.monster!=0xFFFFu || target.marker!=0){ fail(); break; }
        target.feature=8;
        messages.push_back("마력장 생성!");
        break;
    }
    case 9: { // Destroy / DELIO -- one cell in front
        int dx=0,dy=0; delta(d.orientation,dx,dy);
        auto& target=cell(d,d.x+dx,d.y+dy);
        if(target.feature!=8){ fail(); break; }
        target.feature=0;
        messages.push_back("마력장 파괴!");
        break;
    }
    case 10: { // Kill / INTERFICIO -- one cell in front
        int dx=0,dy=0; delta(d.orientation,dx,dy);
        auto& target=cell(d,d.x+dx,d.y+dy);
        if(target.monster==0xFFFFu){ fail(); break; }
        const int id=static_cast<int>(target.monster);
        finishMonster(target,id," 파괴!");
        break;
    }
    default: break;
    }
}

void DungeonSystem::updateAfterCommand(DungeonState& d,PlayerSave& p,std::vector<std::string>& messages){
    // OUT.EXE 0x8CC9: x-major scan over the 1..9 interior. Monsters already
    // moved this pass have marker >=1000 and are skipped until the cleanup pass.
    for(int x=1;x<=9 && p.hits>0;++x){
        for(int y=1;y<=9 && p.hits>0;++y){
            auto& c=cell(d,x,y);
            if(c.monster==0xFFFF || c.marker>=1000) continue;
            const int dx=d.x-x, dy=d.y-y;
            const int dist=std::abs(dx)+std::abs(dy);
            if(dist==1){ monsterAttack(d,p,static_cast<int>(c.monster),messages); continue; }
            if(dist>=7) continue;
            const int sx=(dx>0)?1:((dx<0)?-1:0);
            const int sy=(dy>0)?1:((dy<0)?-1:0);
            bool moved=false;
            if(sx!=0) moved=moveMonster(d,x,y,sx,0);
            if(!moved && sy!=0) moveMonster(d,x,y,0,sy);
        }
    }
    for(int x=1;x<=9;++x) for(int y=1;y<=9;++y){
        auto& c=cell(d,x,y); if(c.marker>999) c.marker=static_cast<std::uint16_t>(c.marker-1000u);
    }
}


std::vector<std::string> DungeonSystem::visibleObjectMessages(const DungeonState& d) const{
    std::vector<std::string> out;
    if(!d.active) return out;

    int fx,fy; delta(d.orientation,fx,fy);

    // OUT.EXE raw 0x40D4/0x407E/0x4013 + raw 0x3DF5 dispatcher semantics.
    // Do NOT infer the message traversal from a precomputed far depth here.
    // Walk the actual center ray near -> far, record each cell that the
    // dispatcher is allowed to reach, and stop immediately after the first
    // sight blocker.  This makes it impossible for a Chest/Coffin two cells
    // behind an occluding Ranger (or any other ordinary monster) to leak into
    // the command-window discovery text.
    //
    // The original renderer later dispatches the retained cells far -> near,
    // and within a cell prints monster first, then Coffin/Chest.
    std::vector<std::vector<std::string>> visibleCells;
    visibleCells.reserve(5);
    for(int depth=1;depth<=5;++depth){
        const int x=d.x+fx*depth, y=d.y+fy*depth;
        const auto& c=cell(d,x,y);
        std::vector<std::string> names;
        if(c.monster!=0xFFFFu && depth<5)
            names.push_back(monsterName(static_cast<int>(c.monster)));
        if(c.marker==5) names.push_back("관");
        else if(c.marker==4) names.push_back("상자");
        visibleCells.push_back(std::move(names));

        // Features 1/2/3 and every monster except 9/11/20 terminate sight.
        // Include the blocker cell itself (as OUT.EXE does), but never inspect
        // a cell behind it.
        if(dungeonSightBlocker(d,x,y)) break;
    }

    for(auto it=visibleCells.rbegin();it!=visibleCells.rend();++it)
        for(const auto& name:*it) out.push_back(name);

    // raw 0x4401 dispatches the player's current cell once more at depth 0,
    // except while standing inside a revealed secret-door square (feature 3).
    const auto& here=cell(d,d.x,d.y);
    if(here.feature!=3){
        if(here.monster!=0xFFFFu)
            out.push_back(monsterName(static_cast<int>(here.monster)));
        if(here.marker==5) out.push_back("관");
        else if(here.marker==4) out.push_back("상자");
    }

    // One redraw produces one logical command-window stream.  Each entity gets
    // the requested Korean copula suffix; renderer wrapping is the only line
    // break allowed inside this stream.
    if(out.empty()) return out;
    std::string line;
    for(const auto& name:out){
        if(!line.empty()) line+=' ';
        line+=name;
        line+="입니다.";
    }
    return {line};
}

void DungeonSystem::drawMonsterVector(SDL_Renderer* r,int monsterId,int depth,SDL_Color color) const{
    if(monsterId<0 || monsterId>=25 || depth<1 || depth>=5) return;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);

    // OUT.EXE raw 0x3DA5 special-cases monster 9.  It does not consume the
    // stream at DS:1FE2; instead 0x36B0 + 0x3D25 + 0x3D65 draw the four sides
    // of the current perspective rectangle.
    if(monsterId==9){
        const int l=ORIG_LEFT+PX[static_cast<std::size_t>(depth)];
        const int rr=ORIG_RIGHT-PX[static_cast<std::size_t>(depth)];
        const int t=ORIG_TOP+PY[static_cast<std::size_t>(depth)];
        const int b=ORIG_BOTTOM-PY[static_cast<std::size_t>(depth)];
        rect4(r,l,t,rr,b);
        return;
    }

    drawDungeonVector(r,monsterId,depth,color);
}

void DungeonSystem::drawDungeonVector(SDL_Renderer* r,int vectorId,int depth,SDL_Color color) const{
    if(vectorId<0 || vectorId>=30 || depth<1 || depth>5) return;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    // OUT.EXE raw 0xAE91.  Depth 1 is unscaled; depths 2..4 shift signed
    // coordinates by 1..3 bits. Depth 5 has the executable's extra shift quirk.
    int shift=depth-1;
    if(shift>4) shift=4;
    if(shift==4) ++shift;
    const int baseY=original_.dungeonMonsterVectorBaseY(depth);
    int cx=0,cy=0; bool have=false;
    for(const auto& cmd:original_.dungeonVector(vectorId)){
        const int x=160+dosVectorScale(cmd.x,shift);
        const int y=baseY+dosVectorScale(cmd.y,shift);
        if(cmd.move){
            cx=x; cy=y; have=true;
            plotLogical(r,cx,cy);
        }else if(have){
            line(r,cx,cy,x,y); cx=x; cy=y;
        }else{
            cx=x; cy=y; have=true; plotLogical(r,cx,cy);
        }
    }
}

void DungeonSystem::drawFrontFace(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x34B0.  A forward wall is not a rectangle primitive: the
    // routine emits only its upper/lower strokes.  Side-transition routines
    // supply the vertical edges when the corridor state requires them.  At
    // depth 0 the original defensively insets all four coordinates by 8.
    if(depth<0 || depth>5) return;
    const int inset=(depth==0)?8:0;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    const int l=ORIG_LEFT+PX[static_cast<std::size_t>(depth)]+inset;
    const int rr=ORIG_RIGHT-PX[static_cast<std::size_t>(depth)]-inset;
    const int t=ORIG_TOP+PY[static_cast<std::size_t>(depth)]+inset;
    const int b=ORIG_BOTTOM-PY[static_cast<std::size_t>(depth)]-inset;
    line(r,l,t,rr,t);
    line(r,l,b,rr,b);
}

void DungeonSystem::drawSecretFrontFace(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x3A54.  Draw the ordinary front-face rails, then the
    // doorway inset using the next perspective depth.  The depth-0 case is
    // special: its bottom doorway stroke is drawn in black to cut a passage
    // through the lower eye-plane rail before the three white jamb strokes.
    if(depth<0 || depth>=5) return;
    drawFrontFace(r,depth,color);

    const int innerL=ORIG_LEFT+PX[static_cast<std::size_t>(depth+1)];
    const int innerR=ORIG_RIGHT-PX[static_cast<std::size_t>(depth+1)];
    const int innerT=ORIG_TOP+PY[static_cast<std::size_t>(depth+1)];
    const int innerB=ORIG_BOTTOM-PY[static_cast<std::size_t>(depth)]-(depth==0?8:0);

    if(depth==0) SDL_SetRenderDrawColor(r,0,0,0,255);
    else SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    line(r,innerR,innerB,innerL,innerB);

    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    line(r,innerL,innerB,innerL,innerT);
    line(r,innerL,innerT,innerR,innerT);
    line(r,innerR,innerT,innerR,innerB);
}

void DungeonSystem::drawExactBars(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x3722. Three horizontal bars are derived from the exact
    // perspective PX/PY tables, including the depth/2 rounding term.
    // The DOS routine uses its second dungeon line color here; on EGA this is
    // light red (#FF5555), not the ordinary white corridor/vector color.
    if(depth<1 || depth>5) return;
    (void)color;
    constexpr SDL_Color BARS_COLOR{255,85,85,255};
    SDL_SetRenderDrawColor(r,BARS_COLOR.r,BARS_COLOR.g,BARS_COLOR.b,BARS_COLOR.a);
    const int x1=ORIG_LEFT+PX[static_cast<std::size_t>(depth)];
    const int x2=ORIG_RIGHT-PX[static_cast<std::size_t>(depth)];
    const int top=ORIG_TOP+PY[static_cast<std::size_t>(depth)];
    const int bottom=ORIG_BOTTOM-PY[static_cast<std::size_t>(depth)];
    const int spacing=std::abs(top-bottom)/4;
    for(int i=1;i<=3;++i){
        const int y=top+i*spacing+depth/2;
        line(r,x1,y,x2,y);
    }
}

void DungeonSystem::drawLeftWallSegment(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x33B0.  The primitive is only the two converging rails;
    // vertical caps are emitted separately by 0x3B25 when the side class changes.
    if(depth<1 || depth>5) return;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    const int po=depth-1, pi=depth;
    line(r,ORIG_LEFT+PX[po],ORIG_TOP+PY[po],ORIG_LEFT+PX[pi],ORIG_TOP+PY[pi]);
    line(r,ORIG_LEFT+PX[po],ORIG_BOTTOM-PY[po],ORIG_LEFT+PX[pi],ORIG_BOTTOM-PY[pi]);
}

void DungeonSystem::drawRightWallSegment(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x3430.
    if(depth<1 || depth>5) return;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    const int po=depth-1, pi=depth;
    line(r,ORIG_RIGHT-PX[po],ORIG_TOP+PY[po],ORIG_RIGHT-PX[pi],ORIG_TOP+PY[pi]);
    line(r,ORIG_RIGHT-PX[po],ORIG_BOTTOM-PY[po],ORIG_RIGHT-PX[pi],ORIG_BOTTOM-PY[pi]);
}

void DungeonSystem::drawLeftOpenSegment(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x355A.  Open side passages use two horizontal rails at the
    // inner depth rather than the diagonal wall rails of 0x33B0.
    if(depth<1 || depth>5) return;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    const int po=depth-1, pi=depth;
    const int top=ORIG_TOP+PY[pi], bottom=ORIG_BOTTOM-PY[pi];
    line(r,ORIG_LEFT+PX[po],top,ORIG_LEFT+PX[pi],top);
    line(r,ORIG_LEFT+PX[po],bottom,ORIG_LEFT+PX[pi],bottom);
}

void DungeonSystem::drawRightOpenSegment(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x35DC.
    if(depth<1 || depth>5) return;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    const int po=depth-1, pi=depth;
    const int top=ORIG_TOP+PY[pi], bottom=ORIG_BOTTOM-PY[pi];
    line(r,ORIG_RIGHT-PX[po],top,ORIG_RIGHT-PX[pi],top);
    line(r,ORIG_RIGHT-PX[po],bottom,ORIG_RIGHT-PX[pi],bottom);
}

void DungeonSystem::drawLeftSecretSegment(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x37DB.  Revealed-secret side geometry starts with the
    // ordinary wall rails and adds a three-segment inset jamb.  Integer
    // division truncates toward zero exactly as 8086 IDIV does here.
    if(depth<1 || depth>5) return;
    drawLeftWallSegment(r,depth,color);
    const int sxStep=std::abs((PX[depth-1]-PX[depth])/9);
    const int syStep=std::abs((PY[depth-1]-PY[depth])/5);
    const int x0=ORIG_LEFT+PX[depth-1]+2*sxStep;
    const int y0=ORIG_BOTTOM-PY[depth-1]-syStep-1;
    const int y1=ORIG_TOP+PY[depth]-syStep;
    const int x1=ORIG_LEFT+PX[depth-1]+6*sxStep;
    const int y2=ORIG_TOP+PY[depth]+syStep;
    int y3=ORIG_BOTTOM-PY[depth]+2*syStep;
    if(depth==1) y3-=2;
    line(r,x0,y0,x0,y1);
    line(r,x0,y1,x1,y2);
    line(r,x1,y2,x1,y3);
}

void DungeonSystem::drawRightSecretSegment(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x390D, mirror of 0x37DB.
    if(depth<1 || depth>5) return;
    drawRightWallSegment(r,depth,color);
    const int sxStep=std::abs((PX[depth-1]-PX[depth])/9);
    const int syStep=std::abs((PY[depth-1]-PY[depth])/5);
    const int x0=ORIG_RIGHT-PX[depth-1]-2*sxStep;
    const int y0=ORIG_BOTTOM-PY[depth-1]-syStep-1;
    const int y1=ORIG_TOP+PY[depth]-syStep;
    const int x1=ORIG_RIGHT-PX[depth-1]-6*sxStep;
    const int y2=ORIG_TOP+PY[depth]+syStep;
    int y3=ORIG_BOTTOM-PY[depth]+2*syStep;
    if(depth==1) y3-=2;
    line(r,x0,y0,x0,y1);
    line(r,x0,y1,x1,y2);
    line(r,x1,y2,x1,y3);
}

void DungeonSystem::drawLeftByFeature(SDL_Renderer* r,std::uint16_t feature,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x3F4C: 1/2 -> wall primitive, 3 -> revealed-secret
    // primitive, everything else -> open-passage primitive.  The revealed
    // secret primitive begins with the ordinary wall rails; its decorative
    // doorway face is already emitted by the exact 0x3A54 front-face path.
    if(feature==1 || feature==2) drawLeftWallSegment(r,depth,color);
    else if(feature==3) drawLeftSecretSegment(r,depth,color);
    else drawLeftOpenSegment(r,depth,color);
}

void DungeonSystem::drawRightByFeature(SDL_Renderer* r,std::uint16_t feature,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x3F19.
    if(feature==1 || feature==2) drawRightWallSegment(r,depth,color);
    else if(feature==3) drawRightSecretSegment(r,depth,color);
    else drawRightOpenSegment(r,depth,color);
}

void DungeonSystem::drawLeftTransition(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x3B25.
    if(depth<0 || depth>5) return;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    const int x=ORIG_LEFT+PX[static_cast<std::size_t>(depth)];
    line(r,x,ORIG_TOP+PY[static_cast<std::size_t>(depth)],
           x,ORIG_BOTTOM-PY[static_cast<std::size_t>(depth)]);
}

void DungeonSystem::drawRightTransition(SDL_Renderer* r,int depth,SDL_Color color) const{
    // OUT.EXE raw 0x3B65.
    if(depth<0 || depth>5) return;
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    const int x=ORIG_RIGHT-PX[static_cast<std::size_t>(depth)];
    line(r,x,ORIG_TOP+PY[static_cast<std::size_t>(depth)],
           x,ORIG_BOTTOM-PY[static_cast<std::size_t>(depth)]);
}

int DungeonSystem::sideClass(std::uint16_t feature){
    // OUT.EXE raw 0x411C: 0,6,7,8 are the open class; 1..5 are the closed class.
    return (feature==0 || feature==6 || feature==7 || feature==8)?0:1;
}

bool DungeonSystem::dungeonSightBlocker(const DungeonState& d,int x,int y) const{
    // OUT.EXE raw 0x407E + 0x4013.  Features 1/2/3 stop the forward ray.
    // Most monsters do too, except Gelatinous Cube(9), Chest(11), and
    // Invisible Seeker(20), which the original explicitly excludes here.
    const auto& c=cell(d,x,y);
    if(c.feature==1 || c.feature==2 || c.feature==3) return true;
    if(c.monster==0xFFFFu) return false;
    return c.monster!=9u && c.monster!=11u && c.monster!=20u;
}

int DungeonSystem::sightDepth(const DungeonState& d,int fx,int fy) const{
    // OUT.EXE raw 0x40D4.  Start one square ahead and stop at the first
    // blocker, clipping the result to the five perspective depths.
    int ox=fx, oy=fy, depth=1;
    while(!dungeonSightBlocker(d,d.x+ox,d.y+oy)){
        ++depth;
        ox+=(fx<0?-1:(fx>0?1:0));
        oy+=(fy<0?-1:(fy>0?1:0));
        if(depth>5) return 5;
    }
    return std::min(depth,5);
}

void DungeonSystem::drawWireframe(SDL_Renderer* r,const DungeonState& d,SDL_Color color) const{
    // M12 ports the state structure of OUT.EXE raw 0x414A..0x4568 rather than
    // drawing every side wall independently.  Side primitives are selected by
    // feature class, transition verticals appear only when that class changes,
    // and the forward visibility depth includes the original monster blockers.
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    int fx,fy,lx,ly; delta(d.orientation,fx,fy); leftDelta(d.orientation,lx,ly);

    const auto& here=cell(d,d.x,d.y);
    if(here.feature==1 || here.feature==2){
        // OUT.EXE raw 0x4245: defensive embedded-wall branch.
        drawFrontFace(r,0,color);
        return;
    }

    const int far=sightDepth(d,fx,fy);
    const bool insideSecret=(here.feature==3);

    // OUT.EXE raw 0x427E..0x42E1 has a dedicated renderer state while the
    // player occupies a revealed secret-door square.  M133 skipped this and
    // rendered the near side walls as an ordinary corridor, producing broken
    // wall rails.  If the square directly ahead is wall/hidden-wall/secret,
    // the original draws a blocking eye-plane face and terminates.  Otherwise
    // it draws the depth-0 secret-door frame and continues into the corridor.
    if(insideSecret){
        const auto& ahead=cell(d,d.x+fx,d.y+fy);
        if(ahead.feature==1 || ahead.feature==2 || ahead.feature==3){
            drawFrontFace(r,0,color);
            if(here.marker==4) drawDungeonVector(r,11,1,color);
            else if(here.marker==5) drawDungeonVector(r,25,1,color);
            return;
        }
        drawSecretFrontFace(r,0,color);
    }
    // OUT.EXE raw 0x4411..0x44FC has an occluding-monster-only side-wall
    // continuation after the main far->near pass.  Keep the monster test itself
    // separate from feature blocking: raw 0x4013 examines only the monster slot.
    const auto& farCell=cell(d,d.x+fx*far,d.y+fy*far);
    const bool farBlockedByMonster=(farCell.monster!=0xFFFFu && farCell.monster!=9u &&
                                    farCell.monster!=11u && farCell.monster!=20u);
    std::array<int,6> leftClass{};
    std::array<int,6> rightClass{};
    std::array<std::uint16_t,6> leftFeature{};
    std::array<std::uint16_t,6> rightFeature{};

    // Segment d leads from forward distance d-1 to d.  This is the coordinate
    // convention used by 0x33B0/3430/355A/35DC and by the 0x414A dispatcher.
    for(int depth=1;depth<=far;++depth){
        const int bx=d.x+fx*(depth-1), by=d.y+fy*(depth-1);
        leftFeature[depth]=cell(d,bx+lx,by+ly).feature;
        rightFeature[depth]=cell(d,bx-lx,by-ly).feature;
        leftClass[depth]=sideClass(leftFeature[depth]);
        rightClass[depth]=sideClass(rightFeature[depth]);
    }

    // OUT.EXE traverses far -> near.  Preserve that ordering; it matters where
    // diagonal rails and transition caps share pixels.
    int fartherLeft=-1, fartherRight=-1;
    for(int depth=far;depth>=1;--depth){
        // raw 0x42FF..0x43CC: while standing in a revealed secret doorway,
        // depth 1 deliberately skips both side dispatchers and their class
        // transition logic.  This is the missing state behind both M133
        // screenshots: ordinary near-side rails made the wall appear broken.
        const bool skipNearSecretSides=(insideSecret && depth==1);
        if(!skipNearSecretSides){
            drawLeftByFeature(r,leftFeature[depth],depth,color);
            drawRightByFeature(r,rightFeature[depth],depth,color);

            if(fartherLeft!=-1 && fartherLeft!=leftClass[depth]) drawLeftTransition(r,depth,color);
            if(fartherRight!=-1 && fartherRight!=rightClass[depth]) drawRightTransition(r,depth,color);
            fartherLeft=leftClass[depth]; fartherRight=rightClass[depth];
        }

        // OUT.EXE raw 0x3DF5 is mutually exclusive at the center-cell layer:
        // if a monster is present, it jumps directly to the monster dispatcher
        // (raw 0x3D6C) and skips feature rendering entirely.  Only an empty
        // monster slot dispatches feature 1..8.  This matters for a monster
        // standing on a ladder/bars/door square: the feature must NOT show
        // through the monster.  Marker 5/4 checks still run afterwards.
        const auto& c=cell(d,d.x+fx*depth,d.y+fy*depth);
        if(c.monster!=0xFFFFu){
            drawMonsterVector(r,static_cast<int>(c.monster),depth,color);
        }else{
            if(c.feature==1 || c.feature==2){
                drawFrontFace(r,depth,color);
            }else if(c.feature==3){
                drawSecretFrontFace(r,depth,color);
            }else if(c.feature==8){
                drawExactBars(r,depth,color);
            }

            if(c.feature==6 || c.feature==7){
                const bool northSouth=(d.orientation==3 || d.orientation==4);
                // The raw 0x3DF5 cell dispatcher increments the perspective
                // argument before calling either ladder wrapper.
                drawDungeonVector(r,c.feature==7 ? 26 : 27,depth+1,color);
                drawDungeonVector(r,northSouth ? 28 : 29,depth+1,color);
            }
        }
        // Marker 4/5 wrappers are called with depth+1 in raw 0x3DF5.
        if(c.marker==4) drawDungeonVector(r,11,depth+1,color);
        else if(c.marker==5) drawDungeonVector(r,25,depth+1,color);
    }

    // OUT.EXE raw 0x4401 calls the ordinary cell dispatcher once more for the
    // player's current square with perspective depth 0.  The ladder/chest/coffin
    // wrappers inside raw 0x3DF5 then increment that argument, so objects under
    // the player are drawn at visual depth 1.  M132 omitted this current-cell
    // dispatch, which made ladders (and floor treasure markers) disappear as
    // soon as the player stepped onto their square.
    //
    // A revealed secret doorway (feature 3) follows the dedicated depth-0
    // path restored above and deliberately skips this 0x4401 dispatch; the
    // cases below are the remaining occupiable current-square contents.
    if(here.feature==6 || here.feature==7){
        const bool northSouth=(d.orientation==3 || d.orientation==4);
        drawDungeonVector(r,here.feature==7 ? 26 : 27,1,color);
        drawDungeonVector(r,northSouth ? 28 : 29,1,color);
    }
    if(here.marker==4) drawDungeonVector(r,11,1,color);
    else if(here.marker==5) drawDungeonVector(r,25,1,color);

    // OUT.EXE raw 0x4411..0x44FC: when the far blocker is an ordinary
    // occluding monster, the renderer does NOT stop the side geometry at that
    // depth.  If the monster is nearer than depth 5 and is not sharing a
    // revealed-secret feature, it samples the cells immediately left/right of
    // the monster and draws one additional side segment at perspective far+1.
    // This is why original DOS screenshots retain substantially more corridor
    // wall around a near monster than the pre-M180 clean-room renderer did.
    //
    // The two transition calls below intentionally test only whether the newly
    // sampled side is open, matching raw 0x44CE..0x44FC exactly; this is not the
    // ordinary adjacent-class transition rule used by the main loop above.
    if(far>=1 && farBlockedByMonster && farCell.feature!=3 && far<5){
        const int bx=d.x+fx*far, by=d.y+fy*far;
        const auto extraLeftFeature=cell(d,bx+lx,by+ly).feature;
        const auto extraRightFeature=cell(d,bx-lx,by-ly).feature;
        drawLeftByFeature(r,extraLeftFeature,far+1,color);
        drawRightByFeature(r,extraRightFeature,far+1,color);
        if(sideClass(extraLeftFeature)==0) drawLeftTransition(r,far,color);
        if(sideClass(extraRightFeature)==0) drawRightTransition(r,far,color);
    }else if(far>=1){
        // raw 0x44FE/0x450C ordinary far-side closure.
        if(leftClass[far]) drawLeftTransition(r,far,color);
        if(rightClass[far]) drawRightTransition(r,far,color);
    }
}

