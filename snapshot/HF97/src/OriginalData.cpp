#include "OriginalData.h"
#include "NameEncoding.h"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iterator>
#include <sstream>

namespace {
std::vector<std::uint8_t> readAll(const std::string& p) {
    std::ifstream f(p,std::ios::binary);
    if (!f) return {};
    return {std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>()};
}
std::uint16_t le16(const std::vector<std::uint8_t>& b, std::size_t o) {
    return static_cast<std::uint16_t>(b[o] | (static_cast<std::uint16_t>(b[o+1])<<8));
}
SDL_Color ega(int i) {
    static constexpr SDL_Color p[16] = {
        {0,0,0,255},{0,0,170,255},{0,170,0,255},{0,170,170,255},
        {170,0,0,255},{170,0,170,255},{170,85,0,255},{170,170,170,255},
        {85,85,85,255},{85,85,255,255},{85,255,85,255},{85,255,255,255},
        {255,85,85,255},{255,85,255,255},{255,255,85,255},{255,255,255,255}
    };
    return p[i&15];
}

SDL_Texture* decode16x16(SDL_Renderer* r, const std::vector<std::uint8_t>& raw, int tile, std::string& error) {
    SDL_Surface* s=SDL_CreateRGBSurfaceWithFormat(0,16,16,32,SDL_PIXELFORMAT_RGBA32);
    if(!s){error=SDL_GetError();return nullptr;}
    auto* px=static_cast<std::uint32_t*>(s->pixels);
    const auto base=static_cast<std::size_t>(tile)*128;
    for(int y=0;y<16;++y){
        for(int x=0;x<16;++x){
            const int byte=x/8, bit=7-(x%8);
            int idx=0;
            for(int plane=0;plane<4;++plane){
                const auto v=raw[base + y*8 + plane*2 + byte];
                if(v&(1u<<bit)) idx|=(1<<plane); // B,G,R,I plane order
            }
            auto c=ega(idx);
            px[y*(s->pitch/4)+x]=SDL_MapRGBA(s->format,c.r,c.g,c.b,255);
        }
    }
    SDL_Texture* out=SDL_CreateTextureFromSurface(r,s);
    SDL_FreeSurface(s);
    if(!out){error=SDL_GetError();return nullptr;}
    SDL_SetTextureBlendMode(out,SDL_BLENDMODE_NONE);
    return out;
}


SDL_Texture* decodeCastle16(SDL_Renderer* r, const std::vector<std::uint8_t>& raw, std::string& error) {
    // ULTIMA.EXE CASTLE.16 is exactly 320x200 packed 4-bpp: high nibble then low nibble.
    if(raw.size()!=32000){ error="CASTLE.16 must be 32000 bytes"; return nullptr; }
    SDL_Surface* s=SDL_CreateRGBSurfaceWithFormat(0,320,200,32,SDL_PIXELFORMAT_RGBA32);
    if(!s){ error=SDL_GetError(); return nullptr; }
    auto* px=static_cast<std::uint32_t*>(s->pixels);
    std::size_t pos=0;
    for(int y=0;y<200;++y){
        for(int x=0;x<320;x+=2){
            const auto v=raw[pos++];
            // ULTIMA.EXE remaps CASTLE.16 through a title-only EGA crossover
            // palette.  The rest of the game uses the normal EGA palette.
            static constexpr int cross[16]={0,8,7,15,15,2,10,6,12,7,13,11,1,9,7,15};
            auto c0=ega(cross[v>>4]), c1=ega(cross[v&15]);
            px[y*(s->pitch/4)+x]=SDL_MapRGBA(s->format,c0.r,c0.g,c0.b,255);
            px[y*(s->pitch/4)+x+1]=SDL_MapRGBA(s->format,c1.r,c1.g,c1.b,255);
        }
    }
    SDL_Texture* out=SDL_CreateTextureFromSurface(r,s);
    SDL_FreeSurface(s);
    if(!out){ error=SDL_GetError(); return nullptr; }
    SDL_SetTextureBlendMode(out,SDL_BLENDMODE_NONE);
    return out;
}


SDL_Texture* decodeLauncherLogo(SDL_Renderer* r,const std::vector<std::uint8_t>& exe,std::array<std::uint8_t,275*64>& mask,std::string& error){
    // ULTIMA.EXE C27/B26 draws the final ORIGIN SYSTEMS INC. logo from two
    // per-row pointer tables at DS:3262 (199 columns) and DS:4320 (76 columns).
    // The display is 275x64 pixels at source x=20..294, y=20..83.  The pointer
    // tables deliberately repeat/select rows from the underlying ASCII '*' masks,
    // so decoding the raw 54 source rows directly (M41/M42) produced the wrong
    // vertical geometry.
    constexpr std::size_t DS_FILE_BASE=0x40D0;
    constexpr std::size_t LEFT_PTR=DS_FILE_BASE+0x3262;
    constexpr std::size_t RIGHT_PTR=DS_FILE_BASE+0x4320;
    constexpr int LW=199, RW=76, W=275, H=64;
    if(exe.size()<RIGHT_PTR+H*2){ error="ULTIMA.EXE logo row tables outside executable"; return nullptr; }
    SDL_Surface* s=SDL_CreateRGBSurfaceWithFormat(0,W,H,32,SDL_PIXELFORMAT_RGBA32);
    if(!s){ error=SDL_GetError(); return nullptr; }
    auto* px=static_cast<std::uint32_t*>(s->pixels);
    const auto white=SDL_MapRGBA(s->format,255,255,255,255);
    const auto black=SDL_MapRGBA(s->format,0,0,0,255);
    auto le16local=[&](std::size_t off)->std::uint16_t{
        return static_cast<std::uint16_t>(exe[off] | (static_cast<std::uint16_t>(exe[off+1])<<8));
    };
    for(int y=0;y<H;++y){
        const std::size_t lp=DS_FILE_BASE+le16local(LEFT_PTR+static_cast<std::size_t>(y)*2);
        const std::size_t rp=DS_FILE_BASE+le16local(RIGHT_PTR+static_cast<std::size_t>(y)*2);
        if(lp+LW>exe.size() || rp+RW>exe.size()){ SDL_FreeSurface(s); error="ULTIMA.EXE logo row pointer outside executable"; return nullptr; }
        for(int x=0;x<W;++x){
            const auto ch=(x<LW)?exe[lp+static_cast<std::size_t>(x)]:exe[rp+static_cast<std::size_t>(x-LW)];
            const bool on=(ch=='*');
            mask[static_cast<std::size_t>(y*W+x)]=on?1u:0u;
            px[y*(s->pitch/4)+x]=on?white:black;
        }
    }
    SDL_Texture* out=SDL_CreateTextureFromSurface(r,s); SDL_FreeSurface(s);
    if(!out){ error=SDL_GetError(); return nullptr; }
    SDL_SetTextureBlendMode(out,SDL_BLENDMODE_NONE);
    return out;
}

SDL_Texture* decode8x8(SDL_Renderer* r, const std::vector<std::uint8_t>& raw, int tile, std::string& error) {
    // EGATOWN.BIN: 51 tiles x 32 bytes. Each row is B,G,R,I one-byte planes.
    SDL_Surface* s=SDL_CreateRGBSurfaceWithFormat(0,8,8,32,SDL_PIXELFORMAT_RGBA32);
    if(!s){error=SDL_GetError();return nullptr;}
    auto* px=static_cast<std::uint32_t*>(s->pixels);
    const auto base=static_cast<std::size_t>(tile)*32;
    for(int y=0;y<8;++y){
        for(int x=0;x<8;++x){
            const int bit=7-x;
            int idx=0;
            for(int plane=0;plane<4;++plane){
                const auto v=raw[base + y*4 + plane];
                if(v&(1u<<bit)) idx|=(1<<plane);
            }
            auto c=ega(idx);
            px[y*(s->pitch/4)+x]=SDL_MapRGBA(s->format,c.r,c.g,c.b,255);
        }
    }
    SDL_Texture* out=SDL_CreateTextureFromSurface(r,s);
    SDL_FreeSurface(s);
    if(!out){error=SDL_GetError();return nullptr;}
    SDL_SetTextureBlendMode(out,SDL_BLENDMODE_NONE);
    return out;
}

LocationKind kindForId(int id) {
    if(id<=32) return LocationKind::Town;
    if(id<=40) return LocationKind::Castle;
    if(id<=48) return LocationKind::Landmark;
    return LocationKind::Dungeon;
}
}

bool OriginalData::loadGameplayOracle(const std::string& d, std::string& error) {
    auto mapPacked=readAll(d+"/MAP.BIN");
    if (mapPacked.size()!=13104) { error="MAP.BIN must be 13104 bytes"; return false; }
    map_.clear(); map_.reserve(MAP_W*MAP_H);
    for (auto b:mapPacked) { map_.push_back(b>>4); map_.push_back(b&0x0F); }
    if (map_.size()!=MAP_W*MAP_H) { error="decoded map size mismatch"; return false; }

    // MONDAIN.EXE startup arena lives at DS:0194, DS file base 0x5B70.
    auto mon=readAll(d+"/MONDAIN.EXE");
    if(mon.size()!=31650) { error="MONDAIN.EXE must be 31,650 bytes"; return false; }
    constexpr std::size_t MON_DS=0x5B70, ARENA=MON_DS+0x0194;
    if(ARENA+mondainInitialArena_.size()>mon.size()){error="MONDAIN arena outside executable";return false;}
    std::copy_n(mon.begin()+static_cast<std::ptrdiff_t>(ARENA),mondainInitialArena_.size(),mondainInitialArena_.begin());

    // M270: successful Get-Gem destruction has an explicit source loop at
    // load-image 0x10EB..0x10FD.  It executes sound(2) exactly five times.
    // Do not choose the nearest sound() call to the state-change anchor: the
    // nearby 0x1081 sound(1) belongs to the failed Get-Gem/BEEP path.
    constexpr std::size_t GEM_LOOP=0x200+0x10EB;
    if(GEM_LOOP+0x15>mon.size() ||
       mon[GEM_LOOP+0]!=0x83 || mon[GEM_LOOP+1]!=0x7E || mon[GEM_LOOP+2]!=0x04 ||
       mon[GEM_LOOP+4]!=0x7D ||
       mon[0x200+0x10F1]!=0xB8 || mon[0x200+0x10F3]!=0x00 ||
       mon[0x200+0x10F4]!=0x50 || mon[0x200+0x10F5]!=0xE8){
        error="MONDAIN gem-destruction sound oracle mismatch"; return false;
    }
    const std::int16_t gemRel=static_cast<std::int16_t>(
        static_cast<std::uint16_t>(mon[0x200+0x10F6]) |
        (static_cast<std::uint16_t>(mon[0x200+0x10F7])<<8));
    if(0x10F8+gemRel!=0x053D){ error="MONDAIN gem sound target mismatch"; return false; }
    mondainGemDestroySoundCount_=static_cast<int>(mon[GEM_LOOP+3]);
    mondainGemDestroySoundId_=static_cast<int>(mon[0x200+0x10F2]);
    if(mondainGemDestroySoundCount_!=5 || mondainGemDestroySoundId_!=2){
        error="MONDAIN gem-destruction sound parameters mismatch"; return false;
    }

    auto nif=readAll(d+"/NIF.BIN");
    if(nif.size()!=nifBitmap_.size()){ error="NIF.BIN must be 6720 bytes"; return false; }
    std::copy(nif.begin(),nif.end(),nifBitmap_.begin());

    tcd_=readAll(d+"/TCD.BIN");
    if(tcd_.size()!=6840) { error="TCD.BIN must be 6840 bytes"; return false; }

    return loadOutOracle(d+"/OUT.EXE",error);
}

bool OriginalData::load(const std::string& d, SDL_Renderer* r, std::string& error) {
    if(!loadGameplayOracle(d,error)) return false;

    auto castle=readAll(d+"/CASTLE.16");
    titleTex_=decodeCastle16(r,castle,error);
    if(!titleTex_) return false;

    auto ultima=readAll(d+"/ULTIMA.EXE");
    if(ultima.size()!=36880){ error="ULTIMA.EXE must be 36,880 bytes"; return false; }
    launcherLogoTex_=decodeLauncherLogo(r,ultima,launcherLogoMask_,error);
    if(!launcherLogoTex_) return false;

    // M53: ULTIMA.EXE title routine 0x673 animates the two castle flags.
    // It chooses one of three 8x8 source masks at DS:0124/0164/01A4 and
    // redraws the top-left 7x7 pixels at source x=123 and x=196, y=55.
    // ULTIMA.EXE's DS image begins at raw file offset 0x40D0 in this build.
    constexpr std::size_t ULTIMA_DS=0x40D0;
    constexpr std::array<std::size_t,3> FLAG_DS{{0x0124,0x0164,0x01A4}};
    for(std::size_t f=0;f<FLAG_DS.size();++f){
        const auto off=ULTIMA_DS+FLAG_DS[f];
        if(off+64>ultima.size()){ error="ULTIMA.EXE title flag frames outside executable"; return false; }
        std::copy_n(ultima.begin()+static_cast<std::ptrdiff_t>(off),64,titleFlagFrames_[f].begin());
    }

    // M29: GEN.EXE input routine 0x43D5 animates a four-frame 8x8 cursor
    // from DS:09FE. GEN.EXE data begins at file 0x4460 in this 1987 build.
    auto gen=readAll(d+"/GEN.EXE");
    if(gen.size()!=20112){ error="GEN.EXE must be 20,112 bytes"; return false; }
    constexpr std::size_t GEN_DS=0x4460, CURSOR=GEN_DS+0x09FE;
    if(CURSOR+32>gen.size()){ error="GEN.EXE cursor frames outside executable"; return false; }
    for(int f=0;f<4;++f)
        std::copy_n(gen.begin()+static_cast<std::ptrdiff_t>(CURSOR+f*8),8,frontendCursor_[static_cast<std::size_t>(f)].begin());

    auto raw=readAll(d+"/EGATILES.BIN");
    if (raw.size()!=6656) { error="EGATILES.BIN must be 6656 bytes"; return false; }
    for (int t=0;t<52;++t) {
        tileTex_[t]=decode16x16(r,raw,t,error);
        if(!tileTex_[t]) return false;
    }

    // M79 / OUT.EXE 0xA754: the overworld water animation mutates only the
    // first 16x16 tile.  In EGA mode each scanline occupies 8 bytes, and each
    // animation quantum rotates the final 8-byte scanline to the front.
    // Tile 0 repeats every four scanlines, so four generated textures reproduce
    // the complete visual cycle without mutating the original asset buffer.
    auto waterRaw=raw;
    for(int frame=0;frame<4;++frame){
        waterAnimTex_[static_cast<std::size_t>(frame)]=decode16x16(r,waterRaw,0,error);
        if(!waterAnimTex_[static_cast<std::size_t>(frame)]) return false;
        if(frame<3){
            std::array<std::uint8_t,8> last{};
            std::copy_n(waterRaw.begin()+120,8,last.begin());
            std::move_backward(waterRaw.begin(),waterRaw.begin()+120,waterRaw.begin()+128);
            std::copy(last.begin(),last.end(),waterRaw.begin());
        }
    }

    auto townRaw=readAll(d+"/EGATOWN.BIN");
    if(townRaw.size()!=1632) { error="EGATOWN.BIN must be 1632 bytes"; return false; }
    for(int t=0;t<51;++t){
        townTex_[t]=decode8x8(r,townRaw,t,error);
        if(!townTex_[t]) return false;
    }

    // M25: EGAMOND.BIN is exactly nineteen 16x16 EGA planar tiles (19*128 bytes).
    auto mondRaw=readAll(d+"/EGAMOND.BIN");
    if(mondRaw.size()!=2432) { error="EGAMOND.BIN must be 2432 bytes"; return false; }
    for(int t=0;t<19;++t){
        mondainTex_[t]=decode16x16(r,mondRaw,t,error);
        if(!mondainTex_[t]) return false;
    }
    return true;
}

bool OriginalData::loadOutOracle(const std::string& path, std::string& error) {
    // These offsets are from the supplied 1987 IBM DOS OUT.EXE (69,858 bytes).
    // Startup sets DS=0x0D48 and the MZ load image starts at file offset 0x200,
    // so DS-relative data begins at file offset 0xD680.
    constexpr std::size_t OUT_SIZE=69858;
    constexpr std::size_t DS_FILE_BASE=0xD680;
    constexpr std::size_t X_TABLE=DS_FILE_BASE+0x18C8; // 84 bytes
    constexpr std::size_t Y_TABLE=DS_FILE_BASE+0x191C; // 84 bytes
    constexpr std::size_t NAME_PTRS=DS_FILE_BASE+0x307C; // word pointers indexed by location id
    constexpr std::size_t PRINCESS_NAME_PTRS=DS_FILE_BASE+0x31CC; // 8 word pointers, castles 33..40
    constexpr std::size_t ACTOR_TABLE=DS_FILE_BASE+0x31DC; // 10 maps * 15 actors * 8 bytes
    constexpr std::size_t DUNGEON_VECTOR_PTRS=DS_FILE_BASE+0x2480; // 25 little-endian DS pointers
    constexpr std::size_t DUNGEON_VECTOR_BASE_Y=DS_FILE_BASE+0x24BC; // five depth baselines

    auto out=readAll(path);
    if(out.size()!=OUT_SIZE){ error="OUT.EXE must be the 69,858-byte 1987 DOS executable"; return false; }
    // Small identity checks so a different executable cannot silently use wrong offsets.
    const std::string britian="Britian";
    const std::string lord="The Castle of Lord British";
    if(DS_FILE_BASE + 0x2B6E + britian.size()>=out.size() ||
       std::string(reinterpret_cast<const char*>(out.data() + DS_FILE_BASE + 0x2B6E),britian.size())!=britian ||
       std::string(reinterpret_cast<const char*>(out.data() + DS_FILE_BASE + 0x2C55),lord.size())!=lord) {
        error="OUT.EXE identity check failed; expected the 1987 DOS data layout"; return false;
    }

    locations_.clear(); locations_.reserve(84);
    for(int id=1;id<=84;++id){
        const int x=out[X_TABLE+static_cast<std::size_t>(id-1)];
        const int y=out[Y_TABLE+static_cast<std::size_t>(id-1)];
        const std::uint16_t ptr=le16(out,NAME_PTRS+static_cast<std::size_t>(id)*2);
        const std::size_t start=DS_FILE_BASE+ptr;
        if(start>=out.size()){error="OUT.EXE location-name pointer outside file";return false;}
        std::size_t end=start;
        while(end<out.size() && out[end]!=0) ++end;
        if(end==out.size()){error="OUT.EXE unterminated location name";return false;}
        LocationInfo info;
        info.id=id; info.x=x; info.y=y; info.kind=kindForId(id);
        info.name.assign(reinterpret_cast<const char*>(out.data()+start),end-start);
        locations_.push_back(std::move(info));
    }

    // Castle princess names are an eight-word pointer table immediately before
    // the local actor templates.  OUT.EXE 0x4E29 indexes it with locationId-33.
    for(int i=0;i<8;++i){
        const std::uint16_t ptr=le16(out,PRINCESS_NAME_PTRS+static_cast<std::size_t>(i)*2);
        const std::size_t start=DS_FILE_BASE+ptr;
        if(start>=out.size()){ error="OUT.EXE princess-name pointer outside file"; return false; }
        std::size_t end=start; while(end<out.size() && out[end]!=0) ++end;
        if(end==out.size()){ error="OUT.EXE unterminated princess name"; return false; }
        princessNames_[static_cast<std::size_t>(i)].assign(reinterpret_cast<const char*>(out.data()+start),end-start);
        const auto first=princessNames_[static_cast<std::size_t>(i)].find_first_not_of(' ');
        if(first!=std::string::npos) princessNames_[static_cast<std::size_t>(i)].erase(0,first);
    }

    // OUT.EXE load-image routine 0x20FC copies 15 records * 8 bytes from
    // DS:31DC + mapIndex*120 into the live actor array at DS:ACC6.
    // Each record is four little-endian words. Rendering/collision use
    // word0=tile, word1=x, word2=y; local combat reads word3 as HP/value.
    if(ACTOR_TABLE + LOCAL_MAP_COUNT*LOCAL_ACTOR_COUNT*8 > out.size()){
        error="OUT.EXE actor template table outside file"; return false;
    }
    for(int mapIndex=0; mapIndex<LOCAL_MAP_COUNT; ++mapIndex){
        for(int actorIndex=0; actorIndex<LOCAL_ACTOR_COUNT; ++actorIndex){
            const std::size_t a=ACTOR_TABLE + static_cast<std::size_t>(mapIndex)*120
                                + static_cast<std::size_t>(actorIndex)*8;
            auto& actor=actorTemplates_[mapIndex][actorIndex];
            actor.tile=le16(out,a+0);
            actor.x=le16(out,a+2);
            actor.y=le16(out,a+4);
            actor.value=le16(out,a+6);
            if(actor.active()){
                if(actor.tile>=51){ error="OUT.EXE actor tile outside EGATOWN range"; return false; }
                if(actor.x>=LOCAL_W || actor.y>=LOCAL_H){ error="OUT.EXE actor coordinate outside local map"; return false; }
            }
        }
    }

    // M10: OUT.EXE raw 0xAE91 consumes one vector stream per dungeon-monster
    // ID through DS:2480.  A stream is made of signed byte coordinate pairs;
    // 0x7E in the X-byte position means move-to and 0x7F ends the stream.
    // Gelatinous cube (ID 9) has a stream too, but the dispatcher deliberately
    // bypasses it and draws the exact perspective rectangle with dedicated code.
    for(int i=0;i<30;++i){
        const std::uint16_t ptr=le16(out,DUNGEON_VECTOR_PTRS+static_cast<std::size_t>(i)*2);
        std::size_t pos=DS_FILE_BASE+ptr;
        if(pos>=DUNGEON_VECTOR_PTRS){ error="OUT.EXE dungeon vector pointer outside vector-data region"; return false; }
        auto& shape=dungeonVectors_[static_cast<std::size_t>(i)];
        shape.clear();
        bool ended=false;
        for(int guard=0;guard<1024 && pos<DUNGEON_VECTOR_PTRS;++guard){
            const std::uint8_t op=out[pos++];
            if(op==0x7F){ ended=true; break; }
            bool move=false;
            std::uint8_t xb=op;
            if(op==0x7E){
                move=true;
                if(pos>=DUNGEON_VECTOR_PTRS){ error="OUT.EXE truncated dungeon vector move command"; return false; }
                xb=out[pos++];
            }
            if(pos>=DUNGEON_VECTOR_PTRS){ error="OUT.EXE truncated dungeon vector coordinate pair"; return false; }
            const std::uint8_t yb=out[pos++];
            const auto s8=[](std::uint8_t b)->std::int8_t {
                return static_cast<std::int8_t>(b<128 ? static_cast<int>(b) : static_cast<int>(b)-256);
            };
            shape.push_back(DungeonVectorCommand{move,s8(xb),s8(yb)});
        }
        if(!ended){ error="OUT.EXE unterminated dungeon vector stream"; return false; }
    }
    for(int i=0;i<5;++i) dungeonMonsterVectorBaseY_[static_cast<std::size_t>(i)]=le16(out,DUNGEON_VECTOR_BASE_Y+static_cast<std::size_t>(i)*2);
    static constexpr std::array<std::uint16_t,5> EXPECTED_BASE_Y={139,112,96,88,84};
    if(dungeonMonsterVectorBaseY_!=EXPECTED_BASE_Y){ error="OUT.EXE dungeon vector scale/baseline table mismatch"; return false; }
    if(!dungeonVectors_[20].empty()){ error="OUT.EXE Invisible seeker vector should be empty"; return false; }

    return true;
}

bool OriginalData::loadSave(const std::string& path, PlayerSave& o, std::string& error) const {
    auto b=readAll(path);
    if(b.size()!=820){error="PLAYER*.U1 must be 820 bytes";return false;}
    std::size_t n=0; while(n<15 && b[n]) ++n; o.name=NameEncoding::fromSave(std::string_view(reinterpret_cast<const char*>(b.data()),n));
    // M49 Korean localization extension: preserve the exact 820-byte DOS save
    // while allowing a UTF-8 display name in a sidecar.  Legacy saves without
    // the sidecar continue to use the original 15-byte name field unchanged.
    {
        std::ifstream nf(path+".krname",std::ios::binary);
        if(nf){ const std::string ext((std::istreambuf_iterator<char>(nf)),std::istreambuf_iterator<char>()); if(!ext.empty()) o.name=ext; }
    }
    o.race=le16(b,0x10); o.klass=le16(b,0x12); o.sex=le16(b,0x14);
    o.hits=le16(b,0x16); o.strength=le16(b,0x18); o.agility=le16(b,0x1A); o.stamina=le16(b,0x1C);
    o.charisma=le16(b,0x1E); o.wisdom=le16(b,0x20); o.intelligence=le16(b,0x22);
    o.coin=le16(b,0x24); o.experience=le16(b,0x26); o.food=le16(b,0x28);
    // OUT.EXE/SPACE.EXE/MONDAIN.EXE status routines clamp these four
    // primary counters to 9999 whenever they are presented. Normalize on
    // load as well so saves produced by older clean-room builds cannot keep
    // an over-cap internal value (which would also distort derived level).
    o.hits=static_cast<std::uint16_t>(std::min<unsigned>(o.hits,9999u));
    o.food=static_cast<std::uint16_t>(std::min<unsigned>(o.food,9999u));
    o.experience=static_cast<std::uint16_t>(std::min<unsigned>(o.experience,9999u));
    o.coin=static_cast<std::uint16_t>(std::min<unsigned>(o.coin,9999u));
    o.readyWeapon=le16(b,0x2A); o.readySpell=le16(b,0x2C); o.readyArmour=le16(b,0x2E); o.transport=le16(b,0x30);
    o.rngSeedWord=le16(b,0x32);
    o.x=le16(b,0x34); o.y=le16(b,0x36);
    for(int i=0;i<9;++i) o.kingQuest[static_cast<std::size_t>(i)]=le16(b,0x3A+static_cast<std::size_t>(i)*2);
    for(int i=0;i<4;++i) o.gems[static_cast<std::size_t>(i)]=le16(b,0x4C+static_cast<std::size_t>(i)*2);
    for(int i=0;i<6;++i)  o.armour[static_cast<std::size_t>(i)]=le16(b,0x54+static_cast<std::size_t>(i)*2);
    for(int i=0;i<16;++i) o.weapons[static_cast<std::size_t>(i)]=le16(b,0x60+static_cast<std::size_t>(i)*2);
    for(int i=0;i<11;++i) o.spells[static_cast<std::size_t>(i)]=le16(b,0x80+static_cast<std::size_t>(i)*2);
    for(int i=0;i<8;++i)  o.transports[static_cast<std::size_t>(i)]=le16(b,0x96+static_cast<std::size_t>(i)*2);
    o.spaceKills=le16(b,0xA6);
    o.steps=static_cast<std::uint32_t>(le16(b,0xAC)) | (static_cast<std::uint32_t>(le16(b,0xAE))<<16);
    o.shuttleStat0=le16(b,0xB0); o.shuttleStat1=le16(b,0xB2);
    for(int i=0;i<WORLD_OBJECT_COUNT;++i){
        for(int j=0;j<8;++j)
            o.worldObjects[static_cast<std::size_t>(i)].w[static_cast<std::size_t>(j)] = le16(b,0xB4+static_cast<std::size_t>(i)*16+static_cast<std::size_t>(j)*2);
    }
    if(o.x>=MAP_W || o.y>=MAP_H){error="save has invalid world coordinates";return false;}
    return true;
}

const std::string& OriginalData::princessNameForCastle(int locationId) const {
    static const std::string empty;
    const int i=locationId-33;
    return (i>=0 && i<8)?princessNames_[static_cast<std::size_t>(i)]:empty;
}

int OriginalData::terrainAt(int x,int y) const {
    if(x<0||y<0||x>=MAP_W||y>=MAP_H) return 3;
    return map_[y*MAP_W+x];
}
SDL_Texture* OriginalData::physicalTexture(int t) const { return (t>=0&&t<52)?tileTex_[t]:nullptr; }
SDL_Texture* OriginalData::mondainTexture(int slot) const {
    // MONDAIN.EXE draws arena *slot numbers*, not raw EGAMOND file indices.
    // Startup overlays EGAMOND.BIN's 19 entries into composite slots 1..19;
    // slot 0 remains the pre-cleared blank cell.  Therefore arena value N
    // maps to EGAMOND file entry N-1.  Slot 20 is the separately staged
    // EGATILES tile 10 and is rendered by MondainSystem's player overlay.
    return (slot>=1&&slot<=19)?mondainTex_[static_cast<std::size_t>(slot-1)]:nullptr;
}
SDL_Texture* OriginalData::terrainTexture(int logical) const {
    static constexpr int base[8]={0,1,2,3,4,6,7,9};
    if(logical<0||logical>7) return nullptr;
    return tileTex_[base[logical]];
}

SDL_Texture* OriginalData::animatedTerrainTexture(int logical,std::uint32_t step) const {
    if(logical<0||logical>7) return nullptr;

    // OUT.EXE 0xB157 is called once after every completed input-wait quantum.
    // Water is rotated one scanline every call by 0xA754.
    if(logical==0)
        return waterAnimTex_[static_cast<std::size_t>(step&3u)];

    // Castle: OUT.EXE 0xB157 only repaints this tile on multiples of
    // three.  At step 3 it paints frame 5 and leaves that image in place
    // through steps 4..5; at step 6 it paints frame 4 and leaves it through
    // steps 7..8.  The resulting visible cadence is therefore 4,4,4 / 5,5,5.
    if(logical==4)
        return tileTex_[((step%6u)>=3u)?5:4];

    // Town: map logical tile 6 normally maps to physical tile 7.
    // 0xB157 draws frame 7 on multiples of four and frame 8 otherwise.
    if(logical==6)
        return tileTex_[(step==0u || (step&3u)==0u)?7:8];

    return terrainTexture(logical);
}

const LocationInfo* OriginalData::locationAt(int worldX,int worldY) const {
    for(const auto& l:locations_) if(l.x==worldX && l.y==worldY) return &l;
    return nullptr;
}

int OriginalData::localMapIndexForLocation(const LocationInfo& l) const {
    // Recovered directly from OUT.EXE:
    // town setup 0x5063: ((locationId-1) % 8) + 2
    // castle setup 0x4FBF: (locationId-1) % 2
    if(l.kind==LocationKind::Town) return ((l.id-1)%8)+2;
    if(l.kind==LocationKind::Castle) return (l.id-1)%2;
    return -1;
}

void OriginalData::localEntryForLocation(const LocationInfo& l,int& x,int& y) const {
    // OUT.EXE town setup: X=0x13, Y=0x11. Castle setup: X=0, Y=9.
    if(l.kind==LocationKind::Town){x=19;y=17;return;}
    if(l.kind==LocationKind::Castle){x=0;y=9;return;}
    x=y=-1;
}

std::uint8_t OriginalData::localRawTile(int mapIndex,int x,int y) const {
    if(mapIndex<0||mapIndex>=LOCAL_MAP_COUNT||x<0||x>=LOCAL_W||y<0||y>=LOCAL_H) return 0x3F;
    // OUT.EXE 0x260E: map*0x2AC + x*0x12 + y.
    return tcd_[static_cast<std::size_t>(mapIndex)*684 + static_cast<std::size_t>(x)*18 + y];
}

int OriginalData::localDisplayTile(int mapIndex,int x,int y) const {
    const auto raw=localRawTile(mapIndex,x,y);
    // OUT.EXE renderer 0x2468: hidden service-floor markers >=0x33 draw as tile 1.
    return raw>=0x33 ? 1 : static_cast<int>(raw);
}

bool OriginalData::localPassable(int mapIndex,int x,int y) const {
    if(x<0||x>=LOCAL_W||y<0||y>=LOCAL_H) return false;
    const auto raw=localRawTile(mapIndex,x,y);
    // OUT.EXE collision 0x2654: tile 1 or any hidden marker >=0x33 is passable.
    // NPC occupancy is a separate layer and intentionally starts in the next milestone.
    return raw==1 || raw>=0x33;
}

SDL_Texture* OriginalData::localTexture(int t) const {
    return (t>=0&&t<51)?townTex_[t]:nullptr;
}

const std::array<LocalActor,15>& OriginalData::actorTemplate(int mapIndex) const {
    static const std::array<LocalActor,15> empty{};
    if(mapIndex<0 || mapIndex>=LOCAL_MAP_COUNT) return empty;
    return actorTemplates_[mapIndex];
}

const char* OriginalData::actorKindName(std::uint16_t tile, bool inTown) {
    // Names are taken from OUT.EXE local attack dispatch at load-image 0x2C87..0x2D48.
    switch(tile){
        case 0x11: return "Guard";
        case 0x14: return "King";
        case 0x15: return "Merchant";
        case 0x16: return "Princess";
        case 0x13: return inTown ? "Bard" : "Jester";
        case 0x32: return "Wench/Lecher";
        default: return "Actor";
    }
}

int OriginalData::weaponRange(int weaponIndex) {
    // OUT.EXE DS:26DE, indexed by readyWeapon. 0 means it cannot be used by the normal Attack command.
    static constexpr int range[16]={1,1,1,1,0,1,1,3,0,0,0,1,3,1,3,3};
    if(weaponIndex<0||weaponIndex>=16) return 0;
    return range[weaponIndex];
}

const char* OriginalData::weaponAttackName(int weaponIndex) {
    // OUT.EXE DS:2804 pointer table used after the exact prefix "Attack with ".
    static constexpr const char* names[16]={
        "hands","dagger","mace","axe","rope","sword","g sword","bow",
        "amulet","wand","staff","triangle","pistol","L sword","phazor","blaster"
    };
    if(weaponIndex<0||weaponIndex>=16) return "";
    return names[weaponIndex];
}

const std::vector<DungeonVectorCommand>& OriginalData::dungeonMonsterVector(int monsterId) const {
    static const std::vector<DungeonVectorCommand> empty;
    if(monsterId<0 || monsterId>=25) return empty;
    return dungeonVectors_[static_cast<std::size_t>(monsterId)];
}

const std::vector<DungeonVectorCommand>& OriginalData::dungeonVector(int vectorId) const {
    static const std::vector<DungeonVectorCommand> empty;
    if(vectorId<0 || vectorId>=30) return empty;
    return dungeonVectors_[static_cast<std::size_t>(vectorId)];
}

int OriginalData::dungeonMonsterVectorBaseY(int depth) const {
    // OUT.EXE 0xAE91 indexes with depth-1, clamped to 4. Dungeon monsters
    // are dispatched only for depth <5, so live calls use depths 1..4.
    int i=depth-1;
    if(i<0) i=4;
    if(i>4) i=4;
    return static_cast<int>(dungeonMonsterVectorBaseY_[static_cast<std::size_t>(i)]);
}