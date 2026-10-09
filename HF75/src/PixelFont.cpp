#include "PixelFont.h"
#include <fstream>

namespace {
constexpr int kChoNoJong[22]   = {0,0,0,0,0,0,0,0,0,1,3,3,3,1,2,4,4,4,2,1,3,0};
constexpr int kChoWithJong[22] = {0,5,5,5,5,5,5,5,5,6,7,7,7,6,6,7,7,7,6,6,7,5};
constexpr int kJungNoJong[20]  = {0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1};
constexpr int kJungWithJong[20]= {0,2,3,3,3,3,3,3,3,3,3,3,3,3,3,3,2,3,3,3};
constexpr int kJongSet[22]      = {0,0,2,0,2,1,2,1,2,3,0,2,1,3,3,1,2,1,3,3,1,1};

template <class T>
bool readExact(const std::string& path, T* dst, std::size_t n) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return false;
    f.read(reinterpret_cast<char*>(dst), static_cast<std::streamsize>(n));
    if (f.gcount() != static_cast<std::streamsize>(n)) return false;
    char extra;
    return !f.get(extra);
}
}

bool PixelFont::load(const std::string& h12Path, const std::string& roboPath) {
    if (!readExact(h12Path, h12_.data(), 11520)) return false;
    if (!readExact(roboPath, robo_.data(), 4096)) return false;
    loaded_ = true;
    return true;
}

bool PixelFont::nextUtf8(std::string_view s, std::size_t& pos, std::uint32_t& cp) {
    if (pos >= s.size()) return false;
    const std::uint8_t a = static_cast<std::uint8_t>(s[pos++]);
    if (a < 0x80) { cp = a; return true; }
    if ((a & 0xE0) == 0xC0 && pos < s.size()) {
        auto b = static_cast<std::uint8_t>(s[pos++]);
        cp = ((a & 0x1F) << 6) | (b & 0x3F); return true;
    }
    if ((a & 0xF0) == 0xE0 && pos + 1 < s.size()) {
        auto b = static_cast<std::uint8_t>(s[pos++]);
        auto c = static_cast<std::uint8_t>(s[pos++]);
        cp = ((a & 0x0F) << 12) | ((b & 0x3F) << 6) | (c & 0x3F); return true;
    }
    cp = '?';
    return true;
}

int PixelFont::widthOf(std::uint32_t cp) {
    // Gameplay typography is intentionally independent from OUT.EXE's
    // structural text-cell coordinates.  The bundled ASCII glyph path is
    // 8 pixels wide while Hangul is 16 pixels wide.  Do not stretch English
    // merely to make an intermediate English screen resemble the DOS layout;
    // Korean layout will be tuned separately on top of the preserved frame,
    // HUD anchors, and status-field rules.
    return cp < 0x80 ? 8 : 16;
}

int PixelFont::measure(std::string_view utf8) const {
    std::size_t p = 0; std::uint32_t cp = 0; int w = 0;
    while (nextUtf8(utf8,p,cp)) w += widthOf(cp);
    return w;
}

std::array<std::uint8_t,32> PixelFont::composeHangul(std::uint32_t cp) const {
    std::array<std::uint8_t,32> out{};
    const std::uint32_t n = cp - 0xAC00;
    const int cho = static_cast<int>(n / (21*28)) + 1;
    const int jung = static_cast<int>((n/28)%21) + 1;
    const int jong = static_cast<int>(n%28);
    int cs=0, vs=0, fs=0;
    if (jong == 0) { cs=kChoNoJong[jung]; vs=kJungNoJong[cho]; }
    else { cs=kChoWithJong[jung]; vs=kJungWithJong[cho]; fs=kJongSet[jung]; }
    const auto& c = h12_[cs*20 + cho];
    const auto& v = h12_[160 + vs*22 + jung];
    const auto& f = h12_[248 + fs*28 + jong];
    for (int i=0;i<32;++i) out[i] = static_cast<std::uint8_t>(c[i] | v[i] | f[i]);
    return out;
}

void PixelFont::drawAscii(SDL_Renderer* r, int x, int y, std::uint8_t c, SDL_Color color) const {
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    const auto& g = robo_[c];
    for (int yy=0; yy<16; ++yy)
        for (int xx=0; xx<8; ++xx)
            if (g[yy] & (0x80u >> xx)) SDL_RenderDrawPoint(r,x+xx,y+yy);
}

void PixelFont::drawHangul(SDL_Renderer* r, int x, int y, const std::array<std::uint8_t,32>& g, SDL_Color color) const {
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,color.a);
    for (int yy=0; yy<16; ++yy) {
        std::uint16_t bits = (static_cast<std::uint16_t>(g[yy*2])<<8) | g[yy*2+1];
        for (int xx=0; xx<16; ++xx)
            if (bits & (0x8000u >> xx)) SDL_RenderDrawPoint(r,x+xx,y+yy);
    }
}


bool PixelFont::drawCompatJamo(SDL_Renderer* r,int x,int y,std::uint32_t cp,SDL_Color color) const {
    int cho=0,jung=0;
    switch(cp){
        case 0x3131:cho=1;break; case 0x3132:cho=2;break; case 0x3134:cho=3;break; case 0x3137:cho=4;break;
        case 0x3138:cho=5;break; case 0x3139:cho=6;break; case 0x3141:cho=7;break; case 0x3142:cho=8;break;
        case 0x3143:cho=9;break; case 0x3145:cho=10;break; case 0x3146:cho=11;break; case 0x3147:cho=12;break;
        case 0x3148:cho=13;break; case 0x3149:cho=14;break; case 0x314A:cho=15;break; case 0x314B:cho=16;break;
        case 0x314C:cho=17;break; case 0x314D:cho=18;break; case 0x314E:cho=19;break;
        default: if(cp>=0x314F && cp<=0x3163) jung=static_cast<int>(cp-0x314F)+1; else return false;
    }
    std::array<std::uint8_t,32> g{};
    if(cho) g=h12_[cho];              // initial-consonant set 0, glyph 1..19
    else g=h12_[160+jung];            // medial-vowel set 0, glyph 1..21
    drawHangul(r,x,y,g,color);
    return true;
}

void PixelFont::draw(SDL_Renderer* r, int x, int y, std::string_view utf8, SDL_Color color) const {
    if (!loaded_) return;
    std::size_t p=0; std::uint32_t cp=0;
    while (nextUtf8(utf8,p,cp)) {
        if (cp < 0x80) { drawAscii(r,x,y,static_cast<std::uint8_t>(cp),color); x += 8; }
        else if (cp >= 0xAC00 && cp <= 0xD7A3) { drawHangul(r,x,y,composeHangul(cp),color); x += 16; }
        else if (cp >= 0x3131 && cp <= 0x3163 && drawCompatJamo(r,x,y,cp,color)) { x += 16; }
        else { drawAscii(r,x,y,'?',color); x += 8; }
    }
}

int PixelFont::measureFrontend(std::string_view utf8) const {
    std::size_t p=0; std::uint32_t cp=0; int w=0;
    while(nextUtf8(utf8,p,cp)) {
        if(cp < 0x80) w += 8;
        else if((cp>=0xAC00 && cp<=0xD7A3) || (cp>=0x3131 && cp<=0x3163)) w += 16;
        else w += 8;
    }
    return w;
}

void PixelFont::drawFrontend(SDL_Renderer* r,int x,int y,std::string_view utf8,SDL_Color color) const {
    // Localization typography policy: keep the source font's native width.
    // ASCII/ROBO glyphs are 8x16; Hangul H12 glyphs are 16x16.  Do not
    // stretch English merely to preserve the old English-only layout.
    if(!loaded_) return;
    std::size_t p=0; std::uint32_t cp=0;
    while(nextUtf8(utf8,p,cp)){
        if(cp<0x80){
            drawAscii(r,x,y,static_cast<std::uint8_t>(cp),color);
            x+=8;
        } else if(cp>=0xAC00 && cp<=0xD7A3){
            drawHangul(r,x,y,composeHangul(cp),color);
            x+=16;
        } else if(cp>=0x3131 && cp<=0x3163 && drawCompatJamo(r,x,y,cp,color)){
            x+=16;
        } else {
            drawAscii(r,x,y,'?',color);
            x+=8;
        }
    }
}
