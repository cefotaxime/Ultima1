#include "ShopSystem.h"
#include "KoreanText.h"
#include <algorithm>
#include <cctype>
#include <cstdint>

namespace {
constexpr const char* WEAPONS[16]={
    "맨손","단검","철퇴","도끼","밧줄과 못","검","대검","활과 화살",
    "수호부","지팡이","마법봉","삼각검","권총","광선검","페이저","블라스터"
};
constexpr const char* ARMOUR[6]={"맨몸","가죽 갑옷","사슬 갑옷","판금 갑옷","우주복","반사복"};
constexpr const char* SPELLS[11]={"기도","관 열기","상자 열기","마탄","훔치기","상승 사다리","하강 사다리","순간이동","마력장 생성","마력장 파괴","죽음"};

char lower(char c){ return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
char upper(char c){ return static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }

std::vector<std::string> rumor(int i,const PlayerSave& p){
    switch(i){
        case 1:return {
            "우주 여행에 대해 알아 두십시오!",
            "우주 영웅이 되려면 적 우주선",
            "20기 이상을 파괴해야 합니다!"
        };
        case 2:return {
            조사_처리기("["+std::string(p.sex==0?"꽃뱀":"제비")+"]@을 조심하는 게 좋을 겁니다.")
        };
        case 3:return {
            "공주를 구출한 이는 큰 보상을 얻을 것이며,",
            "8 레벨 우주 영웅에게는",
            "특별한 선물이 주어집니다!"
        };
        case 4:return {"당신은 시간을 거슬러 가야 한다는 걸 아십시오."};
        case 5:return {"당신은 사악한 보석을 파괴해야 합니다!"};
        case 6:return {
            "성과 마을에 존재하는 여러 호수와 연못에는",
            "강력한 마력이 깃들어 있습니다.",
            "소원을 들어준다고 말하는 이들도 있지요."
        };
        case 7:return {"이 게임은 정말 훌륭한 게임이라는 사실을 아셔야 합니다!"};
        case 8:return {
            "천 년도 더 전에 마술사 몬데인이",
            "사악한 보석을 만들었습니다.",
            "그 보석이 있으면 그자는 불멸의 존재가 되어",
            "절대 쓰러지지 않습니다."
        };
        default:return {};
    }
}

std::vector<std::string> rumor8QuestPage(){
    return {
        "울티마의 탐험은 타임머신을 찾아",
        "온 세상을 누비는 것입니다.",
        "타임머신을 발견하면 몬데인이",
        "사악한 보석을 만들기 전의 시대로 돌아가",
        "그를 쓰러뜨려야 합니다!"
    };
}
}

ShopKind ShopSystem::markerKind(std::uint8_t m){
    if(m==0x36||m==0x37) return ShopKind::Armour;
    if(m==0x38||m==0x39) return ShopKind::Food;
    if(m==0x3A||m==0x3B) return ShopKind::Weapon;
    if(m==0x3C) return ShopKind::Magic;
    if(m==0x3D) return ShopKind::Pub;
    if(m==0x3E) return ShopKind::Transport;
    return ShopKind::None;
}
const char* ShopSystem::kindName(ShopKind k){
    switch(k){case ShopKind::Armour:return "Armour";case ShopKind::Food:return "Food";case ShopKind::Weapon:return "Weapons";
        case ShopKind::Magic:return "Magic";case ShopKind::Pub:return "Tavern";case ShopKind::Transport:return "Transport";default:return "";}
}

int ShopSystem::weaponBuyPrice(const PlayerSave& p,int i){ return ((255-static_cast<int>(p.intelligence))*i*i)/256+5; }
int ShopSystem::weaponSellPrice(const PlayerSave& p,int i){ return ((static_cast<int>(p.charisma)+40)*i*i)/256+1; }
int ShopSystem::armourBuyPrice(const PlayerSave& p,int i){ return ((200-static_cast<int>(p.intelligence))/4)*i; }
int ShopSystem::armourSellPrice(const PlayerSave& p,int i){ return (static_cast<int>(p.charisma)/4)*i; }
int ShopSystem::spellBuyPrice(const PlayerSave& p,int i){ return ((200-static_cast<int>(p.wisdom))/32)*i; }
int ShopSystem::foodPackPrice(const PlayerSave& p){ return 5-static_cast<int>(p.intelligence)/20; }
int ShopSystem::transportBuyPrice(const PlayerSave& p,int i){ return ((200-static_cast<int>(p.intelligence))/5)*i*i; }

bool ShopSystem::begin(const OriginalData& original,const LocalState& local,const PlayerSave&,
                       std::vector<std::string>& msg){
    if(!local.active || !local.inTown){ msg.push_back("거래하기?"); return false; }
    msg.push_back("거래하기");
    if(local.alert){ msg.push_back("모두가 당신을 피합니다!"); return false; }
    const auto raw=original.localRawTile(local.mapIndex,local.x,local.y);
    kind_=markerKind(raw);
    if(kind_==ShopKind::None){ msg.push_back("판매대 옆으로 가야 합니다!"); return false; }
    townFamily_=std::clamp(local.mapIndex-2,0,7);
    stage_=TradeStage::BuySell;
    options_.clear(); number_.clear(); popupMessage_.clear(); popupPages_.clear(); pubPostDrinkPending_=false;
    return true;
}

void ShopSystem::buildOptions(const OriginalData& original,const LocalState& local,const PlayerSave& p){
    options_.clear();
    if(kind_==ShopKind::Weapon){
        if(buying_){
            // OUT.EXE 0x6885..0x6907. Even/odd TCD maps sell alternating weapon indices.
            // The number of displayed tiers grows at 1500, 3000, and >3000 command-steps.
            const int start=((local.mapIndex-2)&1)?2:1;
            int q=static_cast<int>((p.steps%32767u)/1500u);
            if(q>3 || p.steps>3000u) q=3;
            const int maxIndex=start+2*(q+1);
            for(int i=start;i<=maxIndex && i<=15;i+=2)
                options_.push_back({static_cast<char>('a'+i),i,WEAPONS[i],weaponBuyPrice(p,i),static_cast<int>(p.weapons[static_cast<std::size_t>(i)])});
        }else{
            int shown=0;
            for(int i=1;i<=15 && shown<8;i++) if(p.weapons[static_cast<std::size_t>(i)]!=0){
                options_.push_back({static_cast<char>('A'+i),i,WEAPONS[i],weaponSellPrice(p,i),static_cast<int>(p.weapons[static_cast<std::size_t>(i)])});
                ++shown;
            }
        }
    }else if(kind_==ShopKind::Armour){
        if(buying_){
            const int hi=p.steps>3000u?5:3;
            for(int i=1;i<=hi;i++) options_.push_back({static_cast<char>('a'+i),i,ARMOUR[i],armourBuyPrice(p,i),static_cast<int>(p.armour[static_cast<std::size_t>(i)])});
        }else{
            for(int i=1;i<=5;i++) if(p.armour[static_cast<std::size_t>(i)]!=0)
                options_.push_back({static_cast<char>('A'+i),i,ARMOUR[i],armourSellPrice(p,i),static_cast<int>(p.armour[static_cast<std::size_t>(i)])});
        }
    }else if(kind_==ShopKind::Magic && buying_){
        const int parity=local.mapIndex&1;
        const int start=parity?2:1;
        const int end=(p.klass==2 ? (parity?10:9) : (parity?6:5));
        for(int i=start;i<=end;i+=2){
            if(i==4) continue; // OUT.EXE deliberately skips Steal in the shop list.
            options_.push_back({static_cast<char>('a'+i),i,SPELLS[i],spellBuyPrice(p,i),static_cast<int>(p.spells[static_cast<std::size_t>(i)])});
        }
    }else if(kind_==ShopKind::Transport && buying_){
        const auto c=nearbyTerrainCounts(original,p); // water, grass, forest; OUT.EXE DS:8B94/96/98
        const int objectCount=worldTransportObjectCount(p);
        const bool shuttleOwned=p.transports[6]!=0;
        const bool closed=(c[0]==0&&c[1]==0&&c[2]==0) ||
                          (shuttleOwned && objectCount==15) ||
                          (!shuttleOwned && objectCount==15 && c[1]==0);
        if(closed) return;
        const bool special=!shuttleOwned && objectCount==15;
        if(!special && (c[2]!=0 || c[1]!=0)){
            options_.push_back({'A',1,운송수단_이름_한글(1),transportBuyPrice(p,1),0});
            options_.push_back({'B',2,운송수단_이름_한글(2),transportBuyPrice(p,2),0});
        }
        if(!special && c[0]!=0){
            options_.push_back({'C',3,운송수단_이름_한글(3),transportBuyPrice(p,3),0});
            options_.push_back({'D',4,운송수단_이름_한글(4),transportBuyPrice(p,4),0});
        }
        if(!special && p.steps>3000u && c[1]!=0)
            options_.push_back({'E',5,운송수단_이름_한글(5),transportBuyPrice(p,5),0});
        if(!shuttleOwned)
            options_.push_back({'F',6,운송수단_이름_한글(6),transportBuyPrice(p,6),0});
    }
}

int ShopSystem::worldObjectLast(const PlayerSave& p){
    for(int i=OriginalData::WORLD_OBJECT_COUNT-1;i>0;--i) if(!p.worldObjects[static_cast<std::size_t>(i)].empty()) return i;
    return 0;
}
int ShopSystem::worldTransportObjectCount(const PlayerSave& p){
    const int last=worldObjectLast(p); int n=0;
    for(int i=0;i<=last;++i) if(p.worldObjects[static_cast<std::size_t>(i)].tile()<=0x11) ++n;
    return n;
}
int ShopSystem::worldObjectAt(const PlayerSave& p,int x,int y){
    const int last=worldObjectLast(p);
    // OUT.EXE transport placement queries world objects starting at record 1,
    // deliberately excluding player record 0. This allows a purchased craft to
    // occupy the player's current overworld coordinate so Board can find it.
    for(int i=1;i<=last;++i){ const auto& o=p.worldObjects[static_cast<std::size_t>(i)]; if(o.x()==x&&o.y()==y) return i; }
    return -1;
}
std::array<int,3> ShopSystem::nearbyTerrainCounts(const OriginalData& original,const PlayerSave& p){
    std::array<int,3> c{};
    for(int dy=-1;dy<=1;++dy) for(int dx=-1;dx<=1;++dx){
        const int x=static_cast<int>(p.x)+dx, y=static_cast<int>(p.y)+dy;
        int code=-1; const int oi=worldObjectAt(p,x,y);
        if(oi>=0) code=p.worldObjects[static_cast<std::size_t>(oi)].tile();
        else if(x>=0&&y>=0&&x<OriginalData::MAP_W&&y<OriginalData::MAP_H){
            const int logical=original.terrainAt(x,y);
            static constexpr int physical[8]={0,1,2,3,4,6,7,9}; code=physical[logical&7];
        }
        if(code>=0&&code<=2) ++c[static_cast<std::size_t>(code)];
    }
    return c;
}

bool ShopSystem::spawnTransport(const OriginalData& original,PlayerSave& p,int i){
    auto c=nearbyTerrainCounts(original,p);
    int terrainClass=1;
    if(i<=2 && c[2]!=0) terrainClass=2;
    else if(i==3||i==4) terrainClass=0;
    else terrainClass=1;
    int sx=0,sy=0; bool found=false;
    for(int dy=-1;dy<=1&&!found;++dy) for(int dx=-1;dx<=1;++dx){
        const int x=static_cast<int>(p.x)+dx, y=static_cast<int>(p.y)+dy;
        if(worldObjectAt(p,x,y)>=0) continue;
        if(x<0||y<0||x>=OriginalData::MAP_W||y>=OriginalData::MAP_H) continue;
        const int logical=original.terrainAt(x,y);
        static constexpr int physical[8]={0,1,2,3,4,6,7,9};
        if(physical[logical&7]==terrainClass){sx=x;sy=y;found=true;break;}
    }
    auto& qty=p.transports[static_cast<std::size_t>(i)]; ++qty; // OUT.EXE 0x7165, before object append.
    if(!found) return false;
    const int last=worldObjectLast(p);
    if(last>=OriginalData::WORLD_OBJECT_COUNT-1) return false; // b702 silently refuses beyond record 39.
    auto& o=p.worldObjects[static_cast<std::size_t>(last+1)]; o.w.fill(0);
    o.w[0]=static_cast<std::uint16_t>(i+10+(i>4?1:0));
    o.w[1]=static_cast<std::uint16_t>(terrainClass); o.w[2]=static_cast<std::uint16_t>(sx); o.w[3]=static_cast<std::uint16_t>(sy);
    if(i==6){p.shuttleStat0=1000;p.shuttleStat1=1000;}
    return true;
}

bool ShopSystem::applyItem(char key,const OriginalData& original,const LocalState&,PlayerSave& p,std::vector<std::string>& msg){
    const char lk=lower(key), uk=upper(key);
    auto it=std::find_if(options_.begin(),options_.end(),[&](const ShopOption& o){return lower(o.key)==lk || upper(o.key)==uk;});
    if(it==options_.end()){ msg.push_back("안 한다"); close(); return true; }
    const int i=it->index, price=it->price;
    if(buying_){
        if(price>static_cast<int>(p.coin)){ msg.push_back("돈이 부족합니다!"); close(); return true; }
        p.coin=static_cast<std::uint16_t>(p.coin-price);
        if(kind_==ShopKind::Weapon){ auto& q=p.weapons[static_cast<std::size_t>(i)]; if(q<255) ++q; }
        else if(kind_==ShopKind::Armour){ auto& q=p.armour[static_cast<std::size_t>(i)]; if(q<255) ++q; }
        else if(kind_==ShopKind::Magic){ auto& q=p.spells[static_cast<std::size_t>(i)]; if(q<255) ++q; }
        else if(kind_==ShopKind::Transport){ spawnTransport(original,p,i); }
        // OUT.EXE: "Sold!" is drawn inside the transaction popup,
        // not appended to the scrolling command log.  Keep the completed
        // command row underneath and enter a dismissible popup-result stage.
        popupMessage_={"판매 완료!"};
        stage_=TradeStage::Message;
        return false;
    }else{
        if(kind_==ShopKind::Weapon){
            auto& q=p.weapons[static_cast<std::size_t>(i)]; if(q==0){msg.push_back("안 한다");close();return true;} --q;
            if(p.readyWeapon==i) p.readyWeapon=0;
        }else if(kind_==ShopKind::Armour){
            auto& q=p.armour[static_cast<std::size_t>(i)]; if(q==0){msg.push_back("안 한다");close();return true;} --q;
            if(p.readyArmour==i) p.readyArmour=0;
        }
        p.coin=static_cast<std::uint16_t>(
            std::min(9999,static_cast<int>(p.coin)+price));
        // Original Done! helper is presented in the same transaction popup.
        // Keep the completed sell command on the live command row until the
        // popup is dismissed, mirroring the Sold! purchase timing.
        popupMessage_={"구매 완료!"};
        stage_=TradeStage::Message;
        return false;
    }
    close();
    return true;
}

void ShopSystem::queuePopupPage(std::vector<std::string> page){
    if(page.empty()) return;
    if(popupMessage_.empty()) popupMessage_=std::move(page);
    else popupPages_.push_back(std::move(page));
}

bool ShopSystem::advancePopupPage(){
    if(popupPages_.empty()) return false;
    popupMessage_=std::move(popupPages_.front());
    popupPages_.erase(popupPages_.begin());
    return true;
}

void ShopSystem::buyAle(LocalState&,PlayerSave& p,LocalSimulation&,std::vector<std::string>&){
    // OUT.EXE 0x642C: choosing Buy in a pub purchases one ale immediately.
    // The first modal popup is shown before the drink counter / seduction / rumor
    // logic runs.  handleKey(Message) performs that post-drink phase when the
    // player dismisses this first page, matching the original wait at 0x64F2.
    popupMessage_.clear(); popupPages_.clear(); pubPostDrinkPending_=false;
    if(p.coin==0){
        popupMessage_={"당신은 무일푼입니다!", "쓸 돈이 생기면 다시 오십시오."};
        stage_=TradeStage::Message;
        return;
    }
    --p.coin;
    popupMessage_={"선술집 주인이 말합니다:", "여기, 시원한 술 한잔 하십시오!"};
    pubPostDrinkPending_=true;
    stage_=TradeStage::Message;
}

bool ShopSystem::handleKey(char key,const OriginalData& original,LocalState& local,PlayerSave& p,
                           LocalSimulation& sim,std::vector<std::string>& msg){
    if(stage_==TradeStage::Idle) return false;
    if(stage_==TradeStage::BuySell){
        const char u=upper(key);
        if(u!='B'&&u!='S'){ msg.push_back("안 한다"); close(); return true; }
        buying_=(u=='B');
        if(kind_==ShopKind::Magic && !buying_){
            // OUT.EXE 0x6E72..0x6E9F sends DS:11A5 through the same framed
            // modal helper (0x23E7), waits for a key, then closes the frame.
            popupMessage_={"죄송하지만 주문은 사지 않습니다!"};
            stage_=TradeStage::Message;
            return false;
        }
        if(kind_==ShopKind::Food && !buying_){
            popupMessage_={"먹던 음식을요? 사절합니다!"};
            stage_=TradeStage::Message;
            return false;
        }
        if(kind_==ShopKind::Pub){
            if(!buying_){
                popupMessage_={"술은 이미 차고 넘치게 있습니다!"};
                stage_=TradeStage::Message;
            }else{
                buyAle(local,p,sim,msg);
            }
            return false;
        }
        if(kind_==ShopKind::Transport && !buying_){
            popupMessage_={"죄송하지만 중고 물품은 거래 안 합니다."};
            stage_=TradeStage::Message;
            return false;
        }
        if(kind_==ShopKind::Food){ stage_=TradeStage::FoodQuantity; number_.clear(); return false; }
        buildOptions(original,local,p);
        if(options_.empty()){
            if(kind_==ShopKind::Weapon&&!buying_) msg.push_back("판매할 무기가 없으신데요!");
            else if(kind_==ShopKind::Armour&&!buying_) msg.push_back("판매할 방어구가 없으신데요!");
            else if(kind_==ShopKind::Transport&&buying_) msg.push_back("오늘 장사는 끝났습니다.");
            else msg.push_back("안 한다");
            close(); return true;
        }
        stage_=TradeStage::Item; return false;
    }
    if(stage_==TradeStage::Message){
        if(pubPostDrinkPending_){
            // OUT.EXE 0x64EE onward executes only after the first ale popup has
            // been acknowledged: increment drink count, possible seduction,
            // then a 74/255 rumor chance.  0x60C9 clears/reuses the same popup
            // for each follow-up message page.
            pubPostDrinkPending_=false;
            popupMessage_.clear();
            ++local.drinks;

            bool adjacent=false;
            for(int dx=-1;dx<=1 && !adjacent;dx++) for(int dy=-1;dy<=1;dy++){
                if(dx==0&&dy==0) continue;
                const auto* a=sim.actorAt(local,local.x+dx,local.y+dy);
                if(a && a->tile==0x32){ adjacent=true; break; }
            }
            if(local.drinks>static_cast<int>(p.stamina)/4 && adjacent){
                p.coin=static_cast<std::uint16_t>(p.coin/2);
                if(p.wisdom>0){ --p.wisdom; if(p.wisdom==4) ++p.wisdom; }
                queuePopupPage({
                    std::string("당신은 ") + (p.sex==0 ? "꽃뱀" : "제비") + "의 유혹에 넘어갑니다!",
                    "길고 뜨거운 밤을 보내고, 당신은 되돌아옵니다."
                });
            }
            if(sim.randomRange(1,255)<75){
                const int i=sim.randomRange(11,89)/10;
                queuePopupPage(rumor(i,p));
                if(i==8) queuePopupPage(rumor8QuestPage());
            }
            if(!popupMessage_.empty()) return false;
            close(); return true;
        }
        if(advancePopupPage()) return false;
        close(); return true;
    }
    if(stage_==TradeStage::Item) return applyItem(key,original,local,p,msg);
    if(stage_==TradeStage::FoodQuantity){
        if(key=='0' || (key!='\b' && key!='\r' && key!='\n' && (key<'1' || key>'9'))){
            msg.push_back("안 산다");
            close();
            return true;
        }
        if(key>='1'&&key<='9'){ if(number_.size()<5) number_.push_back(key); return false; }
        if(key=='\b'){ if(!number_.empty()) number_.pop_back(); return false; }
        if(key=='\r'||key=='\n'){
            int packs=number_.empty()?0:std::stoi(number_);
            const int each=foodPackPrice(p);
            const long long cost=static_cast<long long>(packs)*each;
            if(packs<=0 || cost>p.coin){ msg.push_back("돈이 부족합니다!"); }
            else {
                p.coin=static_cast<std::uint16_t>(p.coin-static_cast<std::uint16_t>(cost));
                p.food=static_cast<std::uint16_t>(
                    std::min(9999,static_cast<int>(p.food)+packs*10));
                // The original Sold! acknowledgement is a popup result here too.
                popupMessage_={"판매 완료!"};
                stage_=TradeStage::Message;
                return false;
            }
            close(); return true;
        }
        return false;
    }
    return false;
}

bool ShopSystem::cancel(std::vector<std::string>& msg){
    if(!active()) return false;
    msg.push_back("안 한다"); close(); return true;
}

std::string ShopSystem::inputLine() const{
    if(stage_==TradeStage::BuySell) return "거래하기-구입(B), 판매(S):  ";
    if(stage_==TradeStage::Item) return std::string("거래하기-")+(buying_?"구입: ":"판매: ");
    // Food quantity is entered inside the original 0x23C2 frame rather than
    // appended to the command-row selector.
    if(stage_==TradeStage::FoodQuantity) return std::string("거래하기-구입: ")+number_;
    return {};
}

