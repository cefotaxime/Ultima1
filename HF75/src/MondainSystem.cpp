#include "MondainSystem.h"
#include "KoreanText.h"
#include <algorithm>
#include <cstdlib>
#include <string_view>

namespace {
constexpr std::uint32_t ENDING_POST_BEEP_DELAY_MS=1000;
constexpr std::uint32_t ENDING_DISSOLVE_MS=1000;
constexpr int ENDING_SCROLL_PX_PER_SECOND=24;
constexpr int ENDING_LINE_H=20;
constexpr int ENDING_START_Y=400;
constexpr int ENDING_SCROLL_STOP_OFFSET=380;
constexpr std::uint32_t ENDING_RESTART_DELAY_MS=5000;

const std::vector<std::string>& endingLines(){
    static const std::vector<std::string> lines={
        "은빛 번개가 비처럼 쏟아지며 몬데인의 죽음을 알립니다.",
        "마술사의 마지막 비명에 실린 신비로운 힘이 울려 퍼지자,",
        "당신이 막아 낸 수많은 운명의 단편들이 주마등처럼",
        "눈앞을 스쳐 지나갑니다. 천 년의 세월이 순식간에 흘러간 직후,",
        "기이한 졸음이 당신에게 쏟아집니다.",
        "",
        "잠에서 깨어난 당신은 낯선 장소에 와 있음을 깨닫습니다.",
        "보랏빛 로브를 입은 위엄이 넘치는 젊은이가 당신을 일으켜 세웁니다.",
        "그 순간, 당신은 수천 명의 사람들이 경외와 찬탄의 눈빛으로",
        "당신을 바라보고 있음을 알게 됩니다.",
        "",
        "\"훌륭한 이여. 그대의 사심 없는 영웅적 행동이 짐의 백성을",
        "구원했구나. 감사의 인사만으로는 그대에게 보답하기에 부족하도다.",
        "나 로드 브리티시는 지금 이 자리에서 명하노라.",
        "소사리아의 온 나라는 이제부터 영원토록",
        "이 영웅을 섬기도록 하라.\""
    };
    return lines;
}

std::size_t utf8GlyphCount(std::string_view text){
    std::size_t count=0;
    for(std::size_t i=0;i<text.size();++count){
        const unsigned char c=static_cast<unsigned char>(text[i]);
        std::size_t n=(c<0x80)?1:((c&0xE0)==0xC0?2:((c&0xF0)==0xE0?3:4));
        i+=std::min(n,text.size()-i);
    }
    return count;
}

std::string utf8Prefix(std::string_view text,std::size_t glyphs){
    std::size_t i=0, count=0;
    while(i<text.size() && count<glyphs){
        const unsigned char c=static_cast<unsigned char>(text[i]);
        std::size_t n=(c<0x80)?1:((c&0xE0)==0xC0?2:((c&0xF0)==0xE0?3:4));
        i+=std::min(n,text.size()-i);
        ++count;
    }
    return std::string(text.substr(0,i));
}

std::size_t endingTotalGlyphs(){
    static const std::size_t total=[](){
        std::size_t n=0;
        for(const auto& line:endingLines()) n+=utf8GlyphCount(line);
        return n;
    }();
    return total;
}
}


bool MondainSystem::adjacent8(int ax,int ay,int bx,int by){
    const int dx=std::abs(ax-bx), dy=std::abs(ay-by);
    return dx<=1 && dy<=1 && (dx||dy);
}

void MondainSystem::engage(MondainState& s) const{
    // MONDAIN.EXE DS:0246.  Physical Attack sets this directly; Gem proximity
    // sets it together with DS:0190.  Merely entering the <7 spell range does
    // not engage Mondain.
    s.engaged=true;
}

void MondainSystem::enter(MondainState& s,PlayerSave&,std::vector<std::string>&) const{
    s=MondainState{}; s.active=true; s.introActive=true;
    s.arena=original_.mondainInitialArena();
    // Exact MONDAIN.EXE initial words: player=(3,4), Mondain=(15,4), HP=1000.
    // The executable's static arena already contains Time Machine tile 1 at (2,4),
    // player marker 20 at (3,4), walls tile 5, and evil gem tile 6 at (14,4).
    // M256 restores the source's first blocking Time Machine narration as the
    // framed modal drawn by MONDAIN.EXE.  The post-page handoff lines are not
    // emitted until Space/Return acknowledges this page.
}

bool MondainSystem::move(MondainState& s,int dx,int dy,std::vector<std::string>& out) const{
    if(!s.active || s.victorious) return false;
    const int nx=s.playerX+dx, ny=s.playerY+dy;
    bool moved=true;
    if(!inside(nx,ny) || (nx==s.mondainX&&ny==s.mondainY) || s.arena[idx(nx,ny)]!=0){
        out.push_back("막혔습니다!");
        moved=false;
    }else{
        if(inside(s.playerX,s.playerY) && s.arena[idx(s.playerX,s.playerY)]==20) s.arena[idx(s.playerX,s.playerY)]=0;
        s.playerX=nx; s.playerY=ny; s.arena[idx(nx,ny)]=20;
    }
    // MONDAIN.EXE 0x0815..0x0829 performs this proximity check after every
    // movement command, including a blocked move.  0x0FC7 tests all eight
    // neighboring cells for Gem tile 6.  Only the first contact (pulse==0)
    // starts DS:0190 at 1 and sets DS:0246 engaged.
    if(!s.gemDestroyed && s.gemPulse==0 && s.gemX>=0 && adjacent8(s.playerX,s.playerY,s.gemX,s.gemY)
       && s.arena[idx(s.gemX,s.gemY)]==6){
        s.gemPulse=1; s.gemPulseDir=1; s.engaged=true;
    }
    return moved;
}

void MondainSystem::placeGemForOracle(MondainState& s,int x,int y) const{
    if(s.gemX>=0&&inside(s.gemX,s.gemY) && s.arena[idx(s.gemX,s.gemY)]==6) s.arena[idx(s.gemX,s.gemY)]=0;
    s.gemX=x; s.gemY=y; s.gemDestroyed=false;
    if(inside(x,y)) s.arena[idx(x,y)]=6;
}

bool MondainSystem::getGem(MondainState& s,PlayerSave& p,std::vector<std::string>& out) const{
    out.push_back("가지기-보석");
    if(!s.active || s.gemDestroyed || s.gemX<0 || !adjacent8(s.playerX,s.playerY,s.gemX,s.gemY) || s.arena[idx(s.gemX,s.gemY)]!=6){
        out.push_back("여기에 없습니다!"); return false;
    }
    // Successful Get explicitly sets DS:0246 before destroying the Gem.
    engage(s); s.gemDestroyed=true; s.gemDestroySfxPending=true;
    s.arena[idx(s.gemX,s.gemY)]=0; s.gemX=s.gemY=-1; s.gemPulse=0;
    const int damage=(static_cast<int>(p.hits)*3)/4;
    p.hits=static_cast<std::uint16_t>(std::max(0,static_cast<int>(p.hits)-damage));
    out.push_back("화상을 입습니다! "+std::to_string(damage)+"의 피해량!");
    out.push_back("보석을 파괴하였습니다!");
    // MONDAIN.EXE 0x1112..0x111F: if Mondain is already in corpse state
    // (DS:0244 == 30), destroying the Gem immediately arms the final-victory
    // flag.  He must not get the false-death recovery path after this point.
    if(s.mondainAnimLimit==30){
        s.mondainDead=true; s.victorious=true; s.victoryPopup=true;
        s.victoryDrawPhase=0;
        s.victoryPopupSfxPending=true;
    }
    return true;
}

void MondainSystem::inform(const MondainState& s,std::vector<std::string>& out) const{
    out.push_back("탐지하기");
    if(s.gemDestroyed){
        out.push_back("보석이 없으니 몬데인의 마법 기운이 현저히 감소하였습니다.");
    }else{
        out.push_back("...몬데인이 사악한 보석을 만들고 있습니다!");
    }
}

void MondainSystem::damageMondain(MondainState& s,int damage,std::vector<std::string>& out) const{
    if(!s.active||s.victorious||damage<=0) return;
    // MONDAIN.EXE's damage routines only subtract HP and print the hit result.
    // The human/bat/corpse transition is centralized in command-end routine
    // 0x19C8, after the player's command has completed.  Do not pre-empt that
    // state machine here.
    s.mondainHp-=damage;
    out.push_back("몬데인에게 적중! "+std::to_string(damage)+"의 피해량!");
}

bool MondainSystem::tryMoveMondain(MondainState& s,int dx,int dy) const{
    const int nx=s.mondainX+dx, ny=s.mondainY+dy;
    if(!inside(nx,ny) || (nx==s.playerX&&ny==s.playerY) || s.arena[idx(nx,ny)]!=0) return false;
    s.mondainX=nx; s.mondainY=ny; return true;
}

void MondainSystem::hurtPlayer(PlayerSave& p,int damage,std::vector<std::string>& out) const{
    damage=std::max(0,damage); if(damage<=0) return;
    out.push_back("적중! "+std::to_string(damage)+"의 피해량!");
    p.hits=static_cast<std::uint16_t>(damage>=static_cast<int>(p.hits)?0:static_cast<int>(p.hits)-damage);
}

void MondainSystem::melee(MondainState& s,PlayerSave& p,LocalSimulation& sim,std::vector<std::string>& out) const{
    s.enemyAttackSfxPending=true;
    // Keep the complete original combat sentence in one logical stream row.
    const int threshold=2*(185-static_cast<int>(p.stamina)/2+static_cast<int>(p.readyArmour)*8);
    if(sim.randomRange(1,255)>threshold){ out.push_back("몬데인의 공격! 빗맞음!"); return; }
    const int base=static_cast<int>(p.hits)/32;
    const int damage=base+sim.randomRange(1,20);
    out.push_back("몬데인의 공격! 적중! "+std::to_string(damage)+"의 피해량!");
    p.hits=static_cast<std::uint16_t>(damage>=static_cast<int>(p.hits)?0:static_cast<int>(p.hits)-damage);
    s.enemyHitSfxPending=true;
}

void MondainSystem::cast(MondainState& s,PlayerSave& p,LocalSimulation& sim,std::vector<std::string>& out) const{
    // All three Mondain spells use source sound(5), distinct from melee sound(3).
    s.enemySpellSfxPending=true;
    const int which=sim.randomRange(1,238)%3;
    if(which==0){
        out.push_back("몬데인이 마탄을 시전합니다!");
        const int threshold=2*(195-(static_cast<int>(p.wisdom)+static_cast<int>(p.intelligence)/2));
        if(sim.randomRange(1,255)>=threshold){ out.push_back("빗맞음!"); return; }
        hurtPlayer(p,sim.randomRange(1,100),out);
        s.enemyHitSfxPending=true;
    }else if(which==1){
        out.push_back("몬데인이 정신 폭발을 시전합니다!");
        if(sim.randomRange(1,200)<=140){ out.push_back("빗맞음!"); return; }
        out.push_back("적중! 능력치 감소!");
        std::uint16_t* stats[6]={&p.strength,&p.agility,&p.stamina,&p.charisma,&p.wisdom,&p.intelligence};
        for(auto* q:stats){ int v=*q; if(v>16)v-=v/8; else if(v>=2)v-=2; *q=static_cast<std::uint16_t>(std::max(0,v)); }
    }else{
        out.push_back("몬데인이 정신 충격을 시전합니다!");
        // MONDAIN psionic shock presents a full-screen negative flash while
        // the attack sound is active.  Keep the flash independent of hit/miss.
        s.psionicInvertPending=true;
        if(sim.randomRange(1,200)<=140){ out.push_back("빗맞음!"); return; }
        int damage=static_cast<int>(p.hits)/32; if(damage>255) damage-=254;
        hurtPlayer(p,damage,out);
        // Original restores the normal palette before damage helper sound(2).
        s.psionicHitAfterRestorePending=true;
    }
}


bool MondainSystem::attack(MondainState& s,PlayerSave& p,LocalSimulation& sim,int direction,std::vector<std::string>& out) const{
    engage(s);
    // The active "Attack with <weapon>:  <direction>" row is owned by main.cpp.
    // MONDAIN.EXE prints it before entering this attack core; do not duplicate it
    // as a result row here.
    int dx=0,dy=0; if(direction==1)dx=-1; else if(direction==2)dx=1; else if(direction==3)dy=-1; else if(direction==4)dy=1; else return false;
    const int range=OriginalData::weaponRange(std::clamp<int>(p.readyWeapon,0,15));
    if(range<=0){out.push_back("안 한다");return true;}
    for(int step=1;step<=range;++step){
        int x=s.playerX+dx*step,y=s.playerY+dy*step;
        if(!inside(x,y))break;
        if(x==s.mondainX&&y==s.mondainY){
            const int chance=static_cast<int>(p.agility)+50;
            if(sim.randomRange(1,100)>chance){out.push_back("빗맞음!");return true;}
            const int upper=std::max(2,static_cast<int>(p.strength)+static_cast<int>(p.readyWeapon)*3);
            s.playerAttackHitSfxPending=true;
            damageMondain(s,sim.randomRange(2,upper),out);return true;
        }
        if(s.arena[idx(x,y)]!=0)break;
    }
    out.push_back("안 한다"); return true;
}


bool MondainSystem::castSpell(MondainState& s,PlayerSave& p,LocalSimulation& sim,int spellId,int direction,std::vector<std::string>& out) const{
    if(!s.active || s.victorious) return false;
    spellId=std::clamp(spellId,0,10);

    // Prayer and the ordinary dungeon utility spells have no final-arena effect.
    if(spellId==0 || spellId==1 || spellId==2 || spellId==4 || spellId==5 || spellId==6){
        s.playerSpellFailSfxPending=true;
        out.push_back("아무런 효과도 없습니다!");
        return true;
    }

    // Blink/Create/Destroy/Kill are Wizard spells in U1.  The final arena keeps
    // that class distinction rather than reusing the field/dungeon success gate.
    if((spellId==7 || spellId==8 || spellId==9 || spellId==10) && p.klass!=2){
        s.playerSpellFailSfxPending=true;
        out.push_back("실패했습니다!");
        return true;
    }

    // MONDAIN.EXE has no global distance gate for final-arena spells.
    // Its dedicated distance<7 helper is called only by Magic Missile and Kill;
    // Blink/Create/Destroy do not call it.
    if(spellId==3 || spellId==10){
        const int dist=std::abs(s.mondainX-s.playerX)+std::abs(s.mondainY-s.playerY);
        if(dist>=7){ s.playerSpellFailSfxPending=true; out.push_back("실패했습니다!"); return true; }
    }

    // Final-arena spell gate: Clerics are automatic; other classes fail 30%.
    if(p.klass!=1 && sim.randomRange(1,100)<=30){ s.playerSpellFailSfxPending=true; out.push_back("실패했습니다!"); return true; }

    if(spellId==3){
        const int upper=std::max(1,static_cast<int>(p.wisdom)+static_cast<int>(p.intelligence));
        const int damage=sim.randomRange(1,upper);
        out.push_back("\"VASTO!\"");
        s.playerSpellSfxPending=true;
        s.playerSpellHitSfxPending=true;
        damageMondain(s,damage,out);
        return true;
    }
    if(spellId==7){
        const int nx=(sim.randomRange(1,255)+sim.randomRange(1,255))%19;
        const int ny=(sim.randomRange(1,255)+sim.randomRange(1,255))%9;
        if(!inside(nx,ny) || s.arena[idx(nx,ny)]!=0 || (nx==s.mondainX&&ny==s.mondainY)){
            s.playerSpellFailSfxPending=true; out.push_back("실패했습니다!"); return true;
        }
        if(inside(s.playerX,s.playerY) && s.arena[idx(s.playerX,s.playerY)]==20) s.arena[idx(s.playerX,s.playerY)]=0;
        s.playerX=nx; s.playerY=ny; s.arena[idx(nx,ny)]=20;
        s.playerSpellSfxPending=true;
        out.push_back("짜잔!");
        return true;
    }
    if(spellId==8 || spellId==9){
        int dx=0,dy=0;
        if(direction==1)dx=-1; else if(direction==2)dx=1; else if(direction==3)dy=-1; else if(direction==4)dy=1;
        if(!dx && !dy){ s.playerSpellFailSfxPending=true; out.push_back("안 한다"); return true; }
        const int tx=s.playerX+dx,ty=s.playerY+dy;
        if(!inside(tx,ty)){ s.playerSpellFailSfxPending=true; out.push_back("아무런 효과도 없습니다!"); return true; }
        if(spellId==8){
            // MONDAIN.EXE load 0x0B8D..0x0BB5: Create requires an empty
            // arena byte and rejects the whole X/Y cross through Mondain.
            // The evil gem needs no special case: its arena byte is 6, hence non-empty.
            if(s.arena[idx(tx,ty)]!=0 || tx==s.mondainX || ty==s.mondainY){
                s.playerSpellFailSfxPending=true; out.push_back("아무런 효과도 없습니다!"); return true;
            }
            s.arena[idx(tx,ty)]=5;
            s.playerSpellSfxPending=true;
        }else{
            // Destroy has no Mondain-axis or distance test in the source; it
            // succeeds exactly on an adjacent arena wall byte (5).
            if(s.arena[idx(tx,ty)]!=5){ s.playerSpellFailSfxPending=true; out.push_back("아무런 효과도 없습니다!"); return true; }
            s.arena[idx(tx,ty)]=0;
            s.playerSpellSfxPending=true;
        }
        out.push_back(spellId==8 ? "성공했습니다." : "짜잔!");
        return true;
    }
    if(spellId==10){
        out.push_back("\"INTERFICIO-NUNC!\"");
        s.playerSpellSfxPending=true;
        s.mondainHp=std::min(32767,std::max(1,s.mondainHp)*2);
        out.push_back("주문이 오히려 몬데인을 더욱 강하게 만듭니다!");
        return true;
    }
    return true;
}

void MondainSystem::animateInputWaitTick(MondainState& s,LocalSimulation& sim) const{
    if(!s.active || s.introActive || s.defeated || s.victorious) return;

    // MONDAIN.EXE 0x1477..0x1509.  The temporary magic/flame effect is drawn
    // as composite slots 14..19 on successive input-wait polls, then cleared.
    if(s.transientStage>=0){
        if(s.transientStage>=6){
            // At DS:0130==6 the source clears the arena byte that was set to
            // 14 when the transient effect spawned (0x14A9..0x14AD).
            if(inside(s.transientX,s.transientY) && s.arena[idx(s.transientX,s.transientY)]==14)
                s.arena[idx(s.transientX,s.transientY)]=0;
            s.transientStage=-1;
            s.transientX=s.transientY=-1;
        }else{
            ++s.transientStage;
        }
    }

    // MONDAIN.EXE 0x14CC..0x1509.  DS:0190 walks 1,2,3,4,5,4,3,2,1...
    // while the gem is active.  The renderer maps it to slots 6..10.
    if(s.gemPulse){
        s.gemPulse+=s.gemPulseDir;
        if(s.gemPulse==1) s.gemPulseDir=1;
        else if(s.gemPulse>4) s.gemPulseDir=-1;
    }

    // MONDAIN.EXE 0x150B..0x155E animates Mondain on the same keyboard-poll
    // cadence.  Healthy state 2 randomly chooses frame 0/1; wounded state 11
    // cycles 0,1,2.  Use the shared DOS RNG just as the source routine does.
    if(s.mondainSprite==2){
        s.mondainAnim=(sim.randomRange(1,200)<100)?1:0;
    }else if(s.mondainSprite==11){
        s.mondainAnim=(s.mondainAnim==2)?0:s.mondainAnim+1;
    }
}

void MondainSystem::updateAfterCommand(MondainState& s,PlayerSave& p,LocalSimulation& sim,std::vector<std::string>& out) const{
    if(!s.active||s.victorious) return;

    // MONDAIN.EXE 0x19C8 runs before the enemy action routine 0x186F.
    // Keep that ordering exact: transient effect -> form/death state -> victory
    // gate -> Mondain action.
    if(s.engaged && s.transientStage<0 && sim.randomRange(1,200)<36){
        // 0x19FC..0x1AB1 retries only occupied random cells (up to 100).  Once
        // it finds an empty cell, an X/Y-axis conflict aborts this spawn attempt
        // outright rather than drawing another random coordinate.
        for(int tries=0;tries<100;++tries){
            int x=(sim.randomRange(1,255)+sim.randomRange(1,255))%19;
            int y=(sim.randomRange(1,255)+sim.randomRange(1,255))%9;
            if(s.arena[idx(x,y)]!=0) continue;
            if(x==s.mondainX||x==s.playerX||y==s.mondainY||y==s.playerY) break;
            s.transientX=x; s.transientY=y; s.transientStage=0;
            // Source 0x1AB1 stores 0x0E in DS:0194.  Therefore the temporary
            // effect is a real occupied arena cell for collision/LOS until the
            // input-wait animation reaches stage 6 and clears it.
            s.arena[idx(x,y)]=14;
            break;
        }
    }

    // 0x1AB6..0x1B3B: form selection is HP-driven.  Composite slots 2/3
    // are human Mondain, slots 11/12/13 are the bat, and slot 4 is the corpse.
    if(s.mondainHp>500){
        s.mondainSprite=2; s.mondainAnimLimit=20;
    }else if(s.mondainHp>0){
        if(s.mondainSprite!=11){
            s.mondainSprite=11; s.mondainAnimLimit=40; s.mondainAnim=0;
        }
    }else if(s.mondainAnimLimit!=30){
        if(!s.gemDestroyed){
            // 0x1AF3 prints "Mondain is dead!" before the Gem-state branch.
            // In the false-death path the following "...or is he?" fragment is
            // localized on the same logical command row, as before.
            out.push_back("몬데인이 사망했습니다!... 아마도?");
            s.mondainDead=false;
        }else{
            // Real death follows the same initial 0x1AF3 death-message print,
            // then sets the victory flag.  Keep the user-approved final-death
            // wording as its own command row before the victory popup.
            out.push_back("몬데인이 죽었습니다");
            // DS:0248 != 0: this is the real death.  Original 0x1B20 sets
            // DS:024A before 0x186F can run, so the corpse can never revive.
            s.mondainDead=true;
            s.victorious=true;
            s.victoryPopup=true;
            s.victoryDrawPhase=0;
            s.victoryPopupSfxPending=true;
        }
        s.mondainSprite=4; s.mondainAnimLimit=30; s.mondainAnim=0;
    }

    // The main loop checks DS:024A immediately after 0x19C8 and enters the
    // victory routine before calling 0x186F.
    if(s.victorious) return;

    if(!s.engaged){
        if(sim.randomRange(1,200)<17) out.push_back("기묘한 영창이 들립니다!");
        return;
    }

    // 0x18A9: a false-death corpse regains exactly 25 HP and returns from
    // Mondain's action routine immediately.  It does not move or attack while
    // lying down.  On the *following completed player command*, 0x19C8 sees
    // HP=25 and changes the corpse to the bat state.
    if(s.mondainSprite==4){
        s.mondainHp+=25;
        return;
    }
    if(s.mondainHp<=0) return;

    int dx=s.mondainX-s.playerX, dy=s.mondainY-s.playerY;
    const int dist=std::abs(dx)+std::abs(dy);
    if(dist==1 && s.mondainAnimLimit==20){
        melee(s,p,sim,out);
        if(p.hits==0){ s.defeated=true; out.push_back("당신은 사망하였습니다!"); }
        return; // source 0x18FB jumps directly to 0x19C3
    }
    if(dist<7 && s.mondainAnimLimit==20 && sim.randomRange(1,255)<128){
        cast(s,p,sim,out);
        if(p.hits==0){ s.defeated=true; out.push_back("당신은 사망하였습니다!"); }
        return; // source 0x1920 likewise ends the action immediately
    }
    if(s.mondainAnimLimit==40){
        auto randomSign=[&](){
            const int v=sim.randomRange(1,255)-128;
            return v<0?-1:(v>0?1:0); // source 0x1565 is sign(), including zero
        };
        if(dx==0)dx=randomSign(); if(dy==0)dy=randomSign();
    }
    auto sign=[](int v){return v<0?-1:(v>0?1:0);};
    bool moved=false;
    if(sim.randomRange(1,255)&1){ moved=tryMoveMondain(s,0,sign(dy)); if(!moved)moved=tryMoveMondain(s,sign(dx),0); }
    else { moved=tryMoveMondain(s,sign(dx),0); if(!moved)moved=tryMoveMondain(s,0,sign(dy)); }
    // 0x19B7: only the bat/flee state heals 5 HP when both movement attempts fail.
    if(!moved && s.mondainAnimLimit==40) s.mondainHp+=5;
}


void MondainSystem::updateEndingPresentation(MondainState& s,std::uint32_t nowMs) const{
    if(!s.active || !s.victorious) return;
    if(s.endingDissolveActive){
        const std::uint32_t elapsed=nowMs-s.endingDissolveStartedMs;
        if(elapsed>=ENDING_POST_BEEP_DELAY_MS+ENDING_DISSOLVE_MS){
            s.endingDissolveActive=false;
            s.victoryPopup=false;
            s.endingActive=true;
            s.endingScrollStarted=true;
            s.endingScrollStartedMs=nowMs;
            s.endingScrollOffset=0;
        }
        return;
    }
    if(!s.endingActive) return;
    if(!s.endingScrollStarted){
        s.endingScrollStarted=true;
        s.endingScrollStartedMs=nowMs;
    }
    if(!s.endingTextComplete){
        const std::uint32_t elapsed=nowMs-s.endingScrollStartedMs;
        s.endingScrollOffset=static_cast<int>((static_cast<std::uint64_t>(elapsed)*ENDING_SCROLL_PX_PER_SECOND)/1000u);
        if(s.endingScrollOffset>=ENDING_SCROLL_STOP_OFFSET){
            s.endingScrollOffset=ENDING_SCROLL_STOP_OFFSET;
            s.endingTextComplete=true;
            s.endingTextCompletedMs=nowMs;
        }
    }else if(!s.endingRestartLineVisible &&
             static_cast<std::uint32_t>(nowMs-s.endingTextCompletedMs)>=ENDING_RESTART_DELAY_MS){
        s.endingRestartLineVisible=true;
    }
}

void MondainSystem::draw(SDL_Renderer* r,const PixelFont& font,const MondainState& s) const{
    constexpr SDL_Color WHITE{255,255,255,255};
    constexpr SDL_Color POPUP_TEXT{85,255,255,255};
    constexpr SDL_Color GEM_RED{255,85,85,255};
    constexpr SDL_Color GEM_GREEN{85,255,85,255};
    constexpr SDL_Color GEM_BLUE{85,85,255,255};

    auto sourceRect=[&](int x,int y,int w,int h,SDL_Color c){
        SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
        SDL_Rect q{x*2,y*2,w*2,h*2}; SDL_RenderFillRect(r,&q);
    };
    constexpr SDL_Color EGA_BLUE{0,0,170,255};
    constexpr int POPUP_CENTER_X=(25+297); // host pixels: source midpoint * 2
    auto drawPopupFrame=[&](){
        sourceRect(22,15,279,129,{0,0,0,255});
        sourceRect(25,18,273,1,EGA_BLUE);
        sourceRect(297,18,1,123,EGA_BLUE);
        sourceRect(25,140,273,1,EGA_BLUE);
        sourceRect(25,18,1,123,EGA_BLUE);
    };
    auto centered=[&](int row,const std::string& text){
        font.draw(r,POPUP_CENTER_X-font.measure(text)/2,row*16,text,POPUP_TEXT);
    };
    // M275: the victory modal does NOT reuse the larger Time Machine frame.
    // MONDAIN.EXE 0x0518 clears source (31,23)..(287,127), then 0x04BE
    // draws the blue outline at (34,26)..(284,124).
    auto drawVictoryPopupFrame=[&](){
        sourceRect(31,23,257,105,{0,0,0,255});
        sourceRect(34,26,251,1,EGA_BLUE);
        sourceRect(284,26,1,99,EGA_BLUE);
        sourceRect(34,124,251,1,EGA_BLUE);
        sourceRect(34,26,1,99,EGA_BLUE);
    };

    // M258: MONDAIN.EXE image 0x1DE6 is a four-page Time Machine
    // narration dispatcher.  It calls page routines 0x1B9B, 0x1CAF, 0x1D53,
    // 0x1D94 and waits after every page.  Every page calls the same 0x047D
    // popup initializer, so the inherited OUT.EXE framebuffer outside the
    // popup remains visible until the fourth page is dismissed.
    if(s.introActive){
        // MONDAIN.EXE 0x1D9B/0x1EAF/0x1F53/0x1F94 constructs each complete
        // narration page synchronously (clear -> frame -> all text), returns,
        // and only then 0x3CE2 draws the continue prompt and starts polling.
        // One host render therefore contains the complete page.
        sourceRect(22,15,279,129,{0,0,0,255});
        sourceRect(25,18,273,1,EGA_BLUE);
        sourceRect(297,18,1,123,EGA_BLUE);
        sourceRect(25,140,273,1,EGA_BLUE);
        sourceRect(25,18,1,123,EGA_BLUE);

        if(s.introPage==0){
            // Source page 1: image 0x1B9B, BIOS rows 3/4/6/9/11/12/14/15/16.
            centered(3,"기체에 들어선 당신은");
            centered(4,"표시된 네 개의 홈을 발견합니다.");

            constexpr int COLOR_COLS[4]={12,17,22,27};
            const char* labels[4]={"붉은색","녹색","푸른색","흰색"};
            // User-approved anchor: the second syllable '은' begins where source R began.
            font.draw(r,COLOR_COLS[0]*16-font.measure("붉"),6*16,labels[0],POPUP_TEXT);
            for(int i=1;i<4;++i){
                const int anchorX=COLOR_COLS[i]*16;
                font.draw(r,anchorX+8-font.measure(labels[i])/2,6*16,labels[i],POPUP_TEXT);
            }
            const SDL_Color gemColors[4]={GEM_RED,GEM_GREEN,GEM_BLUE,WHITE};
            // Source image 0x1C1A prints BIOS character 0x04 at row 7,
            // columns 12/17/22/27 with EGA colours 12/10/9/15.
            static constexpr std::uint8_t BIOS_GEM_04[8]={0x00,0x10,0x38,0x7C,0xFE,0x7C,0x38,0x10};
            auto drawBiosGem=[&](int cellX,int cellY,SDL_Color c){
                SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
                for(int yy=0;yy<8;++yy) for(int xx=0;xx<8;++xx){
                    if((BIOS_GEM_04[yy] & (0x80u>>xx))==0) continue;
                    SDL_Rect q{cellX+xx*2,cellY+yy*2,2,2};
                    SDL_RenderFillRect(r,&q);
                }
            };
            for(int i=0;i<4;++i) drawBiosGem(COLOR_COLS[i]*16,7*16,gemColors[i]);

            centered(9,"알맞은 보석을 각 홈에 끼워 넣습니다.");
            centered(11,"좀 더 살펴본 당신은");
            centered(12,"두 가지 사실을 알게 됩니다.");
            centered(14,"첫째는 '발사'라고 적힌 버튼이 있다는 것,");
            centered(15,"둘째는 당신이 기체 안에 갇혔기 때문에");
            centered(16,"그 버튼을 누를 수밖에 없다는 것입니다!");
        }else if(s.introPage==1){
            // Post-launch page 1 (source page 2): image 0x1CAF.  Keep the source paragraph bands
            // (rows 4..7, 9..10, 12..14) while centering the approved Korean.
            centered(4,"당신이 커다란 검은 버튼을 누르자");
            centered(5,"제어 화면 곳곳에서");
            centered(6,"불빛이 춤을 추기");
            centered(7,"시작합니다.");
            centered(9,"발전기의 진동이");
            centered(10,"당신의 발밑 바닥을 뒤흔듭니다.");
            centered(12,"작은 기체가 떨리고 요동치기 시작하며");
            centered(13,"시간을 가로지르는 당신의 여정이");
            centered(14,"시작됩니다...");
        }else if(s.introPage==2){
            // Post-launch page 2 (source page 3): image 0x1D53, rows 8/9/10.
            centered(8,"잠시 후");
            centered(9,"강력한 마법의 힘이");
            centered(10,"기체 밖으로 당신을 끌어냅니다...");
        }else{
            // Post-launch page 3 (source page 4): image 0x1D94, rows 6/7/8 and 13.
            centered(6,"...이제 당신은");
            centered(7,"사악한 몬데인과");
            centered(8,"정면으로 마주합니다!");
            centered(13,"부디 행운을 빕니다!");
        }
        return;
    }
    // M260: victory/defeat modals are drawn AFTER the arena, preserving the lower
    // command/status panes in main.cpp exactly as MONDAIN.EXE does.  The ending
    // NIF bitmap is a distinct post-victory phase.
    if(s.victorious && s.endingActive){
        // Apple-II-inspired presentation: after the victory popup dissolves,
        // the approved Korean ending body scrolls continuously from the bottom
        // of the same 640x400 game raster toward the top.  Unlike Apple II,
        // restart stays on this same ending screen.
        SDL_SetRenderDrawColor(r,0,0,0,255); SDL_RenderClear(r);
        constexpr int ENDING_X=32;
        int row=0;
        for(const auto& line:endingLines()){
            const int y=ENDING_START_Y + row*ENDING_LINE_H - s.endingScrollOffset;
            if(y>-ENDING_LINE_H && y<400 && !line.empty())
                font.draw(r,ENDING_X,y,line,WHITE);
            ++row;
        }
        if(s.endingRestartLineVisible){
            const std::string restartText="스페이스바를 눌러 게임을 재시작하세요";
            font.draw(r,320-font.measure(restartText)/2,23*16,restartText,POPUP_TEXT);
        }
        return;
    }

    // MONDAIN logical viewport is the same 304x144 = 19*16 by 9*16 grid used by OUT.
    // Arena bytes are composite graphic-slot numbers.  Slot 0 is deliberately
    // blank; startup overlays EGAMOND.BIN entries 0..18 into slots 1..19.
    const int ox=16,oy=16,scale=2;
    for(int x=0;x<19;++x) for(int y=0;y<9;++y){
        int tile=s.arena[idx(x,y)];
        if(x==s.mondainX&&y==s.mondainY) tile=s.mondainSprite+s.mondainAnim;
        if(x==s.transientX&&y==s.transientY&&s.transientStage>=0&&s.transientStage<=6)
            tile=14+std::max(0,s.transientStage-1);
        if(x==s.gemX&&y==s.gemY&&!s.gemDestroyed&&s.gemPulse>0) tile=5+s.gemPulse;
        if(tile>=1&&tile<=19){
            if(auto* tex=original_.mondainTexture(tile)){
                SDL_Rect d{ox+x*16*scale,oy+y*16*scale,16*scale,16*scale};
                SDL_RenderCopy(r,tex,nullptr,&d);
            }
        }
    }
    // M26 exact tile-20 path: startup first reads 11 EGATILES entries into
    // arena slots 10..20, then overlays EGAMOND's 19 entries into slots 1..19.
    // Consequently slot 20 is exactly EGATILES tile 10.
    if(auto* player=original_.physicalTexture(10)){
        SDL_Rect pp{ox+s.playerX*32,oy+s.playerY*32,32,32};
        SDL_RenderCopy(r,player,nullptr,&pp);
    }

    if(s.victorious && s.victoryPopup && s.victoryDrawPhase>=1){
        // MONDAIN.EXE victory routine uses the smaller 0x0518/0x04BE modal,
        // then prints THOU ART VICTORIOUS! at source row 8.
        drawVictoryPopupFrame();
        const auto victoryCentered=[&](int row,const std::string& text){
            font.draw(r,320-font.measure(text)/2,row*16,text,POPUP_TEXT);
        };
        victoryCentered(6,"당신은");
        victoryCentered(8,"몬데인과의 전투에서");
        victoryCentered(10,"승리했습니다!");
    }else if(s.defeated){
        // MONDAIN.EXE 0x14F2 first emits THOU ART DEAD! to the command stream,
        // then opens the framed popup.  Popup body is rows 6/7 and 12.
        drawPopupFrame();
        centered(6,"당신은 마술사 몬데인에게");
        centered(7,"패배하였습니다!");
        centered(12,"우주의 운명은 끝입니다!");
    }
}
