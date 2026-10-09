#include "FrontendSystem.h"
#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstdint>
#include <chrono>
#include <ctime>
#include <string>
#include <string_view>

namespace {
constexpr SDL_Color WHITE{255,255,255,255};
constexpr SDL_Color YELLOW{255,255,85,255};
constexpr SDL_Color BLUE{85,85,255,255};
constexpr SDL_Color FRONT_BLUE{0,0,170,255};     // EGA color 1
constexpr SDL_Color FRONT_CYAN{85,255,255,255}; // EGA color 11
constexpr SDL_Color LAUNCHER_BLUE{85,85,255,255}; // ULTIMA.EXE ORIGIN logo: EGA color 9
constexpr std::uint32_t LOGO_DISSOLVE_MS=900; // host adaptation for CPU-speed-bound ABF/828 reveal
constexpr std::uint32_t LOGO_INWARD_MS=640;  // host adaptation for CPU-speed-bound B26 row-pair redraw

struct IntroKoStrings {
    static constexpr const char* presents = "오리진 시스템이 제공하는...";
    static constexpr const char* release1 = "...최고의 판매고를 올리고 있는";
    static constexpr const char* release2 = "롤플레잉 어드벤처의 최신작...";
    static constexpr const char* masterpiece1 = "...로드 브리티시의";
    static constexpr const char* masterpiece2 = "독창적 판타지 걸작...";
    static constexpr const char* trademark1 = "울티마와 로드 브리티시는";
    static constexpr const char* trademark2 = "리처드 개리엇과 오리진 시스템의";
    static constexpr const char* trademark3 = "등록 상표입니다.";
    static constexpr const char* conversion = "컨버전: 존 파치니";
    static constexpr const char* localization = "한글화: 울티마 오프라인";
};

struct GenKoStrings {
    static constexpr const char* title = "캐릭터 생성하기";
    static constexpr const char* menuDarkestDungeon = "가장 어두운 던전에서";
    static constexpr const char* menuFarthestSpace = "가장 먼 우주까지!";
    static constexpr const char* generate = "a) 새 캐릭터를 생성합니다";
    static constexpr const char* cont = "b) 이전 게임을 지속합니다";
    static constexpr const char* choice = "선택하세요:";
    static constexpr const char* racePrompt = "종족을 선택하세요:";
    static constexpr const char* raceLabel = "종족:";
    static constexpr const char* sexPrompt = "성별을 선택하세요:";
    static constexpr const char* sexLabel = "성별:";
    static constexpr const char* classPrompt = "직업을 선택하세요:";
    static constexpr const char* classLabel = "직업:";
    static constexpr const char* namePrompt = "이름을 입력하세요:";
    static constexpr const char* savePrompt = "이 캐릭터를 저장합니까?(Y/N)";
    static constexpr const char* yes = "예";
    static constexpr const char* points = "남은 포인트를 분배하세요:";
    static constexpr const char* help1 = "상/하 화살표를 사용하여 커서를 이동합니다. 좌/우 화살표를 사용하여";
    static constexpr const char* help2 = "특성치를 키우거나 줄입니다. 스페이스바를 눌러 작업을 종료하거나";
    static constexpr const char* help3 = "ESC를 눌러 메인 메뉴로 돌아갑니다.";
    static constexpr const char* selectionTitle = "캐릭터 선택하기";
    static constexpr const char* selectionHelp1 = "울티마 I에서 사용하고 싶은 캐릭터의 숫자를 입력하세요.";
    static constexpr const char* selectionHelp2 = "ESC를 눌러 메인 매뉴로 돌아갑니다...";
    static constexpr const char* selectionEmpty1 = "이 디스크에는 저장된 울티마 I 캐릭터가 없습니다.";
    static constexpr const char* selectionEmpty2 = "ENTER를 눌러 캐릭터를 생성하거나";
    static constexpr const char* selectionEmpty3 = "ESC를 눌러 메인 매뉴로 돌아갑니다...";
    static constexpr const char* replacementTitle = "캐릭터 교체하기";
    static constexpr const char* replacementHelp1 = "명단이 가득 찼습니다. 새 캐릭터로 교체하고 싶은 캐릭터 번호를 입력하세요.";
    static constexpr const char* replacementHelp2 = "ESC를 눌러 메인 매뉴로 돌아갑니다...";
    static constexpr const char* loading1 = "게임을 로드하는 중";
    static constexpr const char* loading2 = "잠시 기다려 주세요";
};

const char* raceNameKo(int v){
    static constexpr const char* N[4]={"인간","엘프","드워프","보비트"};
    return N[std::clamp(v,0,3)];
}
const char* sexNameKo(int v){ return v?"여성":"남성"; }
const char* classNameKo(int v){
    static constexpr const char* N[4]={"전사","사제","마법사","도적"};
    return N[std::clamp(v,0,3)];
}

void centered(SDL_Renderer* r,const PixelFont& font,int y,const std::string& s,SDL_Color c=WHITE){
    const int x=std::max(0,(640-font.measure(s))/2);
    font.draw(r,x,y,s,c);
}
void label(SDL_Renderer* r,const PixelFont& font,int x,int y,const std::string& s,bool hi=false){
    font.draw(r,x,y,s,hi?YELLOW:WHITE);
}

// IBM PC BIOS-compatible 8x8 basic Latin glyphs, public domain.
// The ULTIMA.EXE attract cards use BIOS INT 10h text, not the GEN localization font.
// Only printable ASCII 0x20..0x7E is stored here. Bit 0 is the leftmost pixel.
static constexpr std::uint8_t BIOS8X8[95][8] = {
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}, // ' '
    {0x18,0x3C,0x3C,0x18,0x18,0x00,0x18,0x00}, // '!'
    {0x36,0x36,0x00,0x00,0x00,0x00,0x00,0x00}, // '"'
    {0x36,0x36,0x7F,0x36,0x7F,0x36,0x36,0x00}, // '#'
    {0x0C,0x3E,0x03,0x1E,0x30,0x1F,0x0C,0x00}, // '$'
    {0x00,0x63,0x33,0x18,0x0C,0x66,0x63,0x00}, // '%'
    {0x1C,0x36,0x1C,0x6E,0x3B,0x33,0x6E,0x00}, // '&'
    {0x06,0x06,0x03,0x00,0x00,0x00,0x00,0x00}, // "'"
    {0x18,0x0C,0x06,0x06,0x06,0x0C,0x18,0x00}, // '('
    {0x06,0x0C,0x18,0x18,0x18,0x0C,0x06,0x00}, // ')'
    {0x00,0x66,0x3C,0xFF,0x3C,0x66,0x00,0x00}, // '*'
    {0x00,0x0C,0x0C,0x3F,0x0C,0x0C,0x00,0x00}, // '+'
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x06}, // ','
    {0x00,0x00,0x00,0x3F,0x00,0x00,0x00,0x00}, // '-'
    {0x00,0x00,0x00,0x00,0x00,0x0C,0x0C,0x00}, // '.'
    {0x60,0x30,0x18,0x0C,0x06,0x03,0x01,0x00}, // '/'
    {0x3E,0x63,0x73,0x7B,0x6F,0x67,0x3E,0x00}, // '0'
    {0x0C,0x0E,0x0C,0x0C,0x0C,0x0C,0x3F,0x00}, // '1'
    {0x1E,0x33,0x30,0x1C,0x06,0x33,0x3F,0x00}, // '2'
    {0x1E,0x33,0x30,0x1C,0x30,0x33,0x1E,0x00}, // '3'
    {0x38,0x3C,0x36,0x33,0x7F,0x30,0x78,0x00}, // '4'
    {0x3F,0x03,0x1F,0x30,0x30,0x33,0x1E,0x00}, // '5'
    {0x1C,0x06,0x03,0x1F,0x33,0x33,0x1E,0x00}, // '6'
    {0x3F,0x33,0x30,0x18,0x0C,0x0C,0x0C,0x00}, // '7'
    {0x1E,0x33,0x33,0x1E,0x33,0x33,0x1E,0x00}, // '8'
    {0x1E,0x33,0x33,0x3E,0x30,0x18,0x0E,0x00}, // '9'
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x00}, // ':'
    {0x00,0x0C,0x0C,0x00,0x00,0x0C,0x0C,0x06}, // ';'
    {0x18,0x0C,0x06,0x03,0x06,0x0C,0x18,0x00}, // '<'
    {0x00,0x00,0x3F,0x00,0x00,0x3F,0x00,0x00}, // '='
    {0x06,0x0C,0x18,0x30,0x18,0x0C,0x06,0x00}, // '>'
    {0x1E,0x33,0x30,0x18,0x0C,0x00,0x0C,0x00}, // '?'
    {0x3E,0x63,0x7B,0x7B,0x7B,0x03,0x1E,0x00}, // '@'
    {0x0C,0x1E,0x33,0x33,0x3F,0x33,0x33,0x00}, // 'A'
    {0x3F,0x66,0x66,0x3E,0x66,0x66,0x3F,0x00}, // 'B'
    {0x3C,0x66,0x03,0x03,0x03,0x66,0x3C,0x00}, // 'C'
    {0x1F,0x36,0x66,0x66,0x66,0x36,0x1F,0x00}, // 'D'
    {0x7F,0x46,0x16,0x1E,0x16,0x46,0x7F,0x00}, // 'E'
    {0x7F,0x46,0x16,0x1E,0x16,0x06,0x0F,0x00}, // 'F'
    {0x3C,0x66,0x03,0x03,0x73,0x66,0x7C,0x00}, // 'G'
    {0x33,0x33,0x33,0x3F,0x33,0x33,0x33,0x00}, // 'H'
    {0x1E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 'I'
    {0x78,0x30,0x30,0x30,0x33,0x33,0x1E,0x00}, // 'J'
    {0x67,0x66,0x36,0x1E,0x36,0x66,0x67,0x00}, // 'K'
    {0x0F,0x06,0x06,0x06,0x46,0x66,0x7F,0x00}, // 'L'
    {0x63,0x77,0x7F,0x7F,0x6B,0x63,0x63,0x00}, // 'M'
    {0x63,0x67,0x6F,0x7B,0x73,0x63,0x63,0x00}, // 'N'
    {0x1C,0x36,0x63,0x63,0x63,0x36,0x1C,0x00}, // 'O'
    {0x3F,0x66,0x66,0x3E,0x06,0x06,0x0F,0x00}, // 'P'
    {0x1E,0x33,0x33,0x33,0x3B,0x1E,0x38,0x00}, // 'Q'
    {0x3F,0x66,0x66,0x3E,0x36,0x66,0x67,0x00}, // 'R'
    {0x1E,0x33,0x07,0x0E,0x38,0x33,0x1E,0x00}, // 'S'
    {0x3F,0x2D,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 'T'
    {0x33,0x33,0x33,0x33,0x33,0x33,0x3F,0x00}, // 'U'
    {0x33,0x33,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 'V'
    {0x63,0x63,0x63,0x6B,0x7F,0x77,0x63,0x00}, // 'W'
    {0x63,0x63,0x36,0x1C,0x1C,0x36,0x63,0x00}, // 'X'
    {0x33,0x33,0x33,0x1E,0x0C,0x0C,0x1E,0x00}, // 'Y'
    {0x7F,0x63,0x31,0x18,0x4C,0x66,0x7F,0x00}, // 'Z'
    {0x1E,0x06,0x06,0x06,0x06,0x06,0x1E,0x00}, // '['
    {0x03,0x06,0x0C,0x18,0x30,0x60,0x40,0x00}, // '\\'
    {0x1E,0x18,0x18,0x18,0x18,0x18,0x1E,0x00}, // ']'
    {0x08,0x1C,0x36,0x63,0x00,0x00,0x00,0x00}, // '^'
    {0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF}, // '_'
    {0x0C,0x0C,0x18,0x00,0x00,0x00,0x00,0x00}, // '`'
    {0x00,0x00,0x1E,0x30,0x3E,0x33,0x6E,0x00}, // 'a'
    {0x07,0x06,0x06,0x3E,0x66,0x66,0x3B,0x00}, // 'b'
    {0x00,0x00,0x1E,0x33,0x03,0x33,0x1E,0x00}, // 'c'
    {0x38,0x30,0x30,0x3E,0x33,0x33,0x6E,0x00}, // 'd'
    {0x00,0x00,0x1E,0x33,0x3F,0x03,0x1E,0x00}, // 'e'
    {0x1C,0x36,0x06,0x0F,0x06,0x06,0x0F,0x00}, // 'f'
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x1F}, // 'g'
    {0x07,0x06,0x36,0x6E,0x66,0x66,0x67,0x00}, // 'h'
    {0x0C,0x00,0x0E,0x0C,0x0C,0x0C,0x1E,0x00}, // 'i'
    {0x30,0x00,0x30,0x30,0x30,0x33,0x33,0x1E}, // 'j'
    {0x07,0x06,0x66,0x36,0x1E,0x36,0x67,0x00}, // 'k'
    {0x0E,0x0C,0x0C,0x0C,0x0C,0x0C,0x1E,0x00}, // 'l'
    {0x00,0x00,0x33,0x7F,0x7F,0x6B,0x63,0x00}, // 'm'
    {0x00,0x00,0x1F,0x33,0x33,0x33,0x33,0x00}, // 'n'
    {0x00,0x00,0x1E,0x33,0x33,0x33,0x1E,0x00}, // 'o'
    {0x00,0x00,0x3B,0x66,0x66,0x3E,0x06,0x0F}, // 'p'
    {0x00,0x00,0x6E,0x33,0x33,0x3E,0x30,0x78}, // 'q'
    {0x00,0x00,0x3B,0x6E,0x66,0x06,0x0F,0x00}, // 'r'
    {0x00,0x00,0x3E,0x03,0x1E,0x30,0x1F,0x00}, // 's'
    {0x08,0x0C,0x3E,0x0C,0x0C,0x2C,0x18,0x00}, // 't'
    {0x00,0x00,0x33,0x33,0x33,0x33,0x6E,0x00}, // 'u'
    {0x00,0x00,0x33,0x33,0x33,0x1E,0x0C,0x00}, // 'v'
    {0x00,0x00,0x63,0x6B,0x7F,0x7F,0x36,0x00}, // 'w'
    {0x00,0x00,0x63,0x36,0x1C,0x36,0x63,0x00}, // 'x'
    {0x00,0x00,0x33,0x33,0x33,0x3E,0x30,0x1F}, // 'y'
    {0x00,0x00,0x3F,0x19,0x0C,0x26,0x3F,0x00}, // 'z'
    {0x38,0x0C,0x0C,0x07,0x0C,0x0C,0x38,0x00}, // '{'
    {0x18,0x18,0x18,0x00,0x18,0x18,0x18,0x00}, // '|'
    {0x07,0x0C,0x0C,0x38,0x0C,0x0C,0x07,0x00}, // '}'
    {0x6E,0x3B,0x00,0x00,0x00,0x00,0x00,0x00}, // '~'
};

void frontText(SDL_Renderer* r,const PixelFont& font,int col,int row,const std::string& s,SDL_Color c){
    font.drawFrontend(r,col*16,row*16,s,c);
}
void centeredFrontText(SDL_Renderer* r,const PixelFont& font,int row,const std::string& s,SDL_Color c){
    const int w=font.measureFrontend(s);
    font.drawFrontend(r,std::max(0,(640-w)/2),row*16,s,c);
}
void centeredIndentedFrontPair(SDL_Renderer* r,const PixelFont& font,int row,
                               const std::string& line1,const std::string& line2,SDL_Color c){
    // Match ULTIMA.EXE's marketing-card layout: line 2 begins three ASCII
    // columns to the right, the same visible width as the leading "...".
    const int indent=font.measureFrontend("...");
    const int blockW=std::max(font.measureFrontend(line1),indent+font.measureFrontend(line2));
    const int x=std::max(0,(640-blockW)/2);
    font.drawFrontend(r,x,row*16,line1,c);
    font.drawFrontend(r,x+indent,(row+1)*16,line2,c);
}
void biosText(SDL_Renderer* r,int col,int row,const std::string& s,SDL_Color c){
    // ULTIMA.EXE uses INT 10h BIOS 8x8 text in 320x200 graphics mode.
    // Render at source resolution and scale every BIOS pixel 2x into the 640x400 host.
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    int sx=col*8;
    const int sy=row*8;
    for(unsigned char ch: s){
        if(ch<0x20 || ch>0x7E){ sx+=8; continue; }
        const auto& g=BIOS8X8[ch-0x20];
        for(int yy=0;yy<8;++yy){
            for(int xx=0;xx<8;++xx){
                if(g[yy] & (1u<<xx)){
                    SDL_Rect q{(sx+xx)*2,(sy+yy)*2,2,2};
                    SDL_RenderFillRect(r,&q);
                }
            }
        }
        sx+=8;
    }
}
void sourcePixel(SDL_Renderer* r,int x,int y,SDL_Color c){
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_Rect q{x*2,y*2,2,2};
    SDL_RenderFillRect(r,&q);
}
void sourceRect(SDL_Renderer* r,int x1,int y1,int x2,int y2,SDL_Color c){
    if(x2<x1) std::swap(x1,x2);
    if(y2<y1) std::swap(y1,y2);
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    SDL_Rect q{x1*2,y1*2,(x2-x1+1)*2,(y2-y1+1)*2};
    SDL_RenderFillRect(r,&q);
}
void sourceLine(SDL_Renderer* r,int x0,int y0,int x1,int y1,SDL_Color c){
    const int dx=std::abs(x1-x0), sx=x0<x1?1:-1;
    const int dy=-std::abs(y1-y0), sy=y0<y1?1:-1;
    int err=dx+dy;
    for(;;){
        sourcePixel(r,x0,y0,c);
        if(x0==x1 && y0==y1) break;
        const int e2=2*err;
        if(e2>=dy){ err+=dy; x0+=sx; }
        if(e2<=dx){ err+=dx; y0+=sy; }
    }
}
void drawLoadingFrame(SDL_Renderer* r){
    // GEN.EXE 0x3CD7. Coordinates below are the original 320x200
    // inclusive primitive endpoints, expanded exactly 2x for 640x400.
    sourceRect(r,0,0,316,6,FRONT_BLUE);
    sourceRect(r,0,0,7,158,FRONT_BLUE);
    sourceRect(r,312,2,319,158,FRONT_BLUE);
    sourceRect(r,0,152,319,158,FRONT_BLUE);

    sourceLine(r,8,7,311,7,WHITE);
    sourceLine(r,7,7,8,151,WHITE);
    sourceLine(r,312,8,312,151,WHITE);
    sourceLine(r,8,152,311,152,WHITE);
    sourceLine(r,0,159,319,159,WHITE);
    sourceLine(r,241,159,241,199,WHITE);
    sourceLine(r,246,159,246,199,WHITE);

    const SDL_Color BLACK{0,0,0,255};
    for(int i=1;i<=4;++i){
        sourceLine(r,i,0,0,i,BLACK);
        sourceLine(r,319-i,0,319,i,BLACK);
        sourceLine(r,241+i,159,241+i,199,FRONT_BLUE);
    }
    // GEN.EXE 0x3E36..0x3E84 explicitly plots these black corner pixels.
    sourcePixel(r,0,0,BLACK); sourcePixel(r,0,5,BLACK); sourcePixel(r,5,0,BLACK);
    sourcePixel(r,319,0,BLACK); sourcePixel(r,314,0,BLACK); sourcePixel(r,319,5,BLACK);
}
void drawGenFrame(SDL_Renderer* r){
    // GEN.EXE 0x7B2: black field, blue bands, then white lines at
    // x=7/312 and y=7/192 on the original 320x200 raster.
    SDL_SetRenderDrawColor(r,FRONT_BLUE.r,FRONT_BLUE.g,FRONT_BLUE.b,255);
    SDL_RenderClear(r);
    SDL_SetRenderDrawColor(r,0,0,0,255);
    SDL_Rect inner{16,16,608,368}; // source x=8..311, y=8..191
    SDL_RenderFillRect(r,&inner);
    SDL_SetRenderDrawColor(r,255,255,255,255);
    SDL_Rect top{14,14,612,2}, left{14,14,2,372}, right{624,14,2,372}, bottom{14,384,612,2};
    SDL_RenderFillRect(r,&top); SDL_RenderFillRect(r,&left);
    SDL_RenderFillRect(r,&right); SDL_RenderFillRect(r,&bottom);
}
void drawInputCursorPx(SDL_Renderer* r,const OriginalData& original,int ox,int oy,SDL_Color c){
    const int frame=static_cast<int>((SDL_GetTicks()/120u)&3u);
    const auto& bits=original.frontendCursorFrame(frame);
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
    for(int yy=0;yy<8;++yy) for(int xx=0;xx<8;++xx)
        if(bits[static_cast<std::size_t>(yy)]&(1u<<xx)){
            SDL_Rect q{ox+xx*2,oy+yy*2,2,2};
            SDL_RenderFillRect(r,&q);
        }
}
void drawInputCursor(SDL_Renderer* r,const OriginalData& original,int col,int row,SDL_Color c){
    drawInputCursorPx(r,original,col*16,row*16,c);
}
void drawArrowButton(SDL_Renderer* r,const PixelFont& font,int col,int row,bool right){
    const int x=col*16, y=row*16;
    SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
    SDL_Rect top{x,y,16,2}, left{x,y,2,16};
    SDL_RenderFillRect(r,&top); SDL_RenderFillRect(r,&left);
    SDL_SetRenderDrawColor(r,FRONT_BLUE.r,FRONT_BLUE.g,FRONT_BLUE.b,255);
    SDL_Rect bottom{x,y+14,16,2}, rr{x+14,y,2,16};
    SDL_RenderFillRect(r,&bottom); SDL_RenderFillRect(r,&rr);
    std::string g(1,static_cast<char>(right?0x10:0x11));
    font.drawFrontend(r,x,y,g,FRONT_BLUE);
}

std::uint32_t launcherPixelHash(int x,int y){
    // Deterministic stand-in for ULTIMA.EXE's CPU-speed-bound ABF/828 LFSR
    // ordering.  It preserves the original visual semantics: isolated logo
    // pixels appear in a pseudo-random dissolve until the complete mask exists.
    std::uint32_t v=static_cast<std::uint32_t>(x+1)*0x45d9f3bu ^ static_cast<std::uint32_t>(y+1)*0x119de1f3u;
    v^=v>>16; v*=0x45d9f3bu; v^=v>>16;
    return v;
}
template<class Visible>
void drawLauncherLogoMask(SDL_Renderer* r,const OriginalData& original,Visible&& visible){
    const auto& mask=original.launcherLogoMask();
    for(int y=0;y<64;++y) for(int x=0;x<275;++x){
        if(!mask[static_cast<std::size_t>(y*275+x)] || !visible(x,y)) continue;
        sourcePixel(r,20+x,20+y,LAUNCHER_BLUE);
    }
}
void drawLauncherLogo(SDL_Renderer* r,const OriginalData& original){
    drawLauncherLogoMask(r,original,[](int,int){ return true; });
}
void drawLauncherLogoDissolve(SDL_Renderer* r,const OriginalData& original,std::uint32_t elapsed){
    const std::uint32_t t=std::min(elapsed,LOGO_DISSOLVE_MS);
    const std::uint64_t limit=(static_cast<std::uint64_t>(t)*0x100000000ull)/LOGO_DISSOLVE_MS;
    drawLauncherLogoMask(r,original,[&](int x,int y){ return static_cast<std::uint64_t>(launcherPixelHash(x,y))<limit; });
}
void drawLauncherLogoInward(SDL_Renderer* r,const OriginalData& original,std::uint32_t elapsed){
    const std::uint32_t t=std::min(elapsed,LOGO_INWARD_MS);
    const int pairs=std::min(32,static_cast<int>((static_cast<std::uint64_t>(t)*32+LOGO_INWARD_MS-1)/LOGO_INWARD_MS));
    drawLauncherLogoMask(r,original,[&](int,int y){ return y<pairs || y>=64-pairs; });
}

void drawFrameChevron(SDL_Renderer* r,int tipX,int y,bool pointsRight){
    // White chevron with FRONT_BLUE interior, matching GEN's title-frame end caps.
    SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
    for(int i=0;i<8;++i){
        const int span=i<4?i:7-i;
        const int x0=pointsRight?tipX-span:tipX+span;
        SDL_RenderDrawPoint(r,x0,y+i*2);
        SDL_RenderDrawPoint(r,x0,y+i*2+1);
    }
}

void drawFramedGenTitle(SDL_Renderer* r,const PixelFont& font,const std::string& title){
    // Keep GEN.EXE's title treatment (black title gap cut into the blue top
    // frame, with inward-facing white chevrons), but size the localized slot
    // to the actual Korean title.  Every localized GEN title deliberately
    // leaves exactly two ASCII-space widths on each side before the chevrons.
    const int titleW=font.measureFrontend(title);
    constexpr int sidePad=2*8; // exactly two 8 px ASCII spaces per side
    const int slotW=titleW+sidePad*2;
    const int slotX=(640-slotW)/2;
    const int leftCapX=slotX-16;
    const int rightCapX=slotX+slotW;

    SDL_SetRenderDrawColor(r,0,0,0,255);
    SDL_Rect clear{slotX,0,slotW,16};
    SDL_RenderFillRect(r,&clear);

    auto cap=[&](int bx,bool pointsRight){
        SDL_SetRenderDrawColor(r,FRONT_BLUE.r,FRONT_BLUE.g,FRONT_BLUE.b,255);
        SDL_Rect bg{bx,0,16,16}; SDL_RenderFillRect(r,&bg);
        SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
        for(int yy=0;yy<16;++yy){
            const int d=std::abs(7-yy);
            const int inset=7-d;
            const int px=pointsRight ? bx+8+inset : bx+7-inset;
            SDL_RenderDrawPoint(r,px,yy);
        }
        // The lower white frame line reconnects only on the *outer* half of
        // each cap.  Leaving the inner half clear prevents the horizontal line
        // from poking through underneath the inward chevron itself.
        const SDL_Rect mid = pointsRight ? SDL_Rect{bx,14,8,2}
                                         : SDL_Rect{bx+8,14,8,2};
        SDL_RenderFillRect(r,&mid);
    };
    cap(leftCapX,true);
    cap(rightCapX,false);
    font.drawFrontend(r,slotX+sidePad,0,title,WHITE);
}

void drawGenerationTitle(SDL_Renderer* r,const PixelFont& font){
    drawFramedGenTitle(r,font,GenKoStrings::title);
}

void drawSelectionTitle(SDL_Renderer* r,const PixelFont& font){
    drawFramedGenTitle(r,font,GenKoStrings::selectionTitle);
}

void drawReplacementTitle(SDL_Renderer* r,const PixelFont& font){
    drawFramedGenTitle(r,font,GenKoStrings::replacementTitle);
}

void drawNameInputModeIndicator(SDL_Renderer* r,const PixelFont& font,bool korean){
    // Korean localization overlay, intentionally independent of GEN.EXE's
    // original English layout.  It occupies the lower-left frame only while
    // a text field is actively waiting for input, following the convention
    // used by Korean Ultima localizations.  The black patch interrupts the
    // white inner border instead of moving/redrawing the original frame.
    const std::string text=korean?"[한글]":"[영문]";
    const int x=16;
    const int y=384;
    const int w=font.measureFrontend(text);
    SDL_SetRenderDrawColor(r,0,0,0,255);
    SDL_Rect patch{x-2,y,w+4,16};
    SDL_RenderFillRect(r,&patch);
    font.drawFrontend(r,x,y,text,WHITE);
}
void drawStatCursor(SDL_Renderer* r,int x,int y,bool pointsRight){
    // Original attribute selector is a free-standing triangular pointer, not a button.
    SDL_SetRenderDrawColor(r,WHITE.r,WHITE.g,WHITE.b,255);
    for(int yy=0;yy<16;++yy){
        const int half=(yy<8)?yy:(15-yy);
        for(int xx=0;xx<=half;++xx){
            const int px=pointsRight ? x+xx : x+15-xx;
            SDL_RenderDrawPoint(r,px,y+yy);
        }
    }
    SDL_SetRenderDrawColor(r,FRONT_BLUE.r,FRONT_BLUE.g,FRONT_BLUE.b,255);
    for(int yy=2;yy<14;++yy){
        const int half=(yy<8)?yy:(15-yy);
        for(int xx=1;xx<half;++xx){
            const int px=pointsRight ? x+xx : x+15-xx;
            SDL_RenderDrawPoint(r,px,y+yy);
        }
    }
}

struct StatsLayout { int x=0; int width=0; };
StatsLayout drawStats(SDL_Renderer* r,const PixelFont& font,const std::array<int,6>& stats){
    static constexpr const char* N[6]={"힘","민첩","지구력","매력","지혜","지능"};
    // User-selected baseline.  All rows target the exact rendered width of
    // "힘..................10" so the numeric column is perfectly vertical.
    const std::string baseline="힘..................10";
    const int target=font.measureFrontend(baseline);
    const int x=(640-target)/2;
    for(int i=0;i<6;++i){
        const std::string value=std::to_string(stats[static_cast<std::size_t>(i)]);
        const int fixed=font.measureFrontend(N[i])+font.measureFrontend(value);
        const int dotW=std::max(1,font.measureFrontend("."));
        const int dots=std::max(0,(target-fixed)/dotW);
        std::string line=N[i];
        line.append(static_cast<std::size_t>(dots),'.');
        line+=value;
        font.drawFrontend(r,x,(6+i)*16,line,FRONT_CYAN);
    }
    return {x,target};
}

std::size_t nextUtf8Char(const std::string& s,std::size_t i){
    if(i>=s.size()) return i;
    const unsigned char c=static_cast<unsigned char>(s[i]);
    if(c<0x80) return i+1;
    if((c&0xE0)==0xC0 && i+1<s.size()) return i+2;
    if((c&0xF0)==0xE0 && i+2<s.size()) return i+3;
    return i+1;
}
void drawWrappedFrontText(SDL_Renderer* r,const PixelFont& font,int x,int y,int maxWidth,const std::string& text,SDL_Color c){
    std::string line;
    int yy=y;
    for(std::size_t i=0;i<text.size();){
        const std::size_t j=nextUtf8Char(text,i);
        const std::string ch=text.substr(i,j-i);
        std::string candidate=line+ch;
        if(!line.empty() && font.measureFrontend(candidate)>maxWidth){
            font.drawFrontend(r,x,yy,line,c);
            yy+=16;
            line=ch;
            if(line==" ") line.clear();
        } else line=std::move(candidate);
        i=j;
    }
    if(!line.empty()) font.drawFrontend(r,x,yy,line,c);
}

void appendUtf8(std::string& out,std::uint32_t cp){
    if(cp<0x80) out.push_back(static_cast<char>(cp));
    else if(cp<0x800){
        out.push_back(static_cast<char>(0xC0|(cp>>6)));
        out.push_back(static_cast<char>(0x80|(cp&0x3F)));
    } else {
        out.push_back(static_cast<char>(0xE0|(cp>>12)));
        out.push_back(static_cast<char>(0x80|((cp>>6)&0x3F)));
        out.push_back(static_cast<char>(0x80|(cp&0x3F)));
    }
}

std::uint32_t compatL(int l){
    static constexpr std::uint32_t C[19]={0x3131,0x3132,0x3134,0x3137,0x3138,0x3139,0x3141,0x3142,0x3143,0x3145,0x3146,0x3147,0x3148,0x3149,0x314A,0x314B,0x314C,0x314D,0x314E};
    return (l>=0&&l<19)?C[l]:0;
}
std::uint32_t compatV(int v){ return (v>=0&&v<21)?static_cast<std::uint32_t>(0x314F+v):0; }
std::uint32_t syllable(int l,int v,int t){ return 0xAC00u+static_cast<std::uint32_t>((l*21+v)*28+t); }

int finalForL(int l){
    static constexpr int F[19]={1,2,4,7,0,8,16,17,0,19,20,21,22,0,23,24,25,26,27};
    return (l>=0&&l<19)?F[l]:0;
}
int lForFinal(int t){
    switch(t){case 1:return 0;case 2:return 1;case 4:return 2;case 7:return 3;case 8:return 5;case 16:return 6;case 17:return 7;case 19:return 9;case 20:return 10;case 21:return 11;case 22:return 12;case 23:return 14;case 24:return 15;case 25:return 16;case 26:return 17;case 27:return 18;default:return -1;}
}
int combineFinal(int a,int b){
    if(a==1&&b==19)return 3;
    if(a==4&&b==22)return 5;
    if(a==4&&b==27)return 6;
    if(a==8&&b==1)return 9;
    if(a==8&&b==16)return 10;
    if(a==8&&b==17)return 11;
    if(a==8&&b==19)return 12;
    if(a==8&&b==25)return 13;
    if(a==8&&b==26)return 14;
    if(a==8&&b==27)return 15;
    if(a==17&&b==19)return 18;
    return 0;
}
bool splitFinal(int t,int& first,int& nextL){
    switch(t){
        case 3:first=1;nextL=9;return true; case 5:first=4;nextL=12;return true; case 6:first=4;nextL=18;return true;
        case 9:first=8;nextL=0;return true; case 10:first=8;nextL=6;return true; case 11:first=8;nextL=7;return true;
        case 12:first=8;nextL=9;return true; case 13:first=8;nextL=16;return true; case 14:first=8;nextL=17;return true;
        case 15:first=8;nextL=18;return true; case 18:first=17;nextL=9;return true; default:return false;
    }
}
int combineVowel(int a,int b){
    if(a==8&&b==0)return 9;
    if(a==8&&b==1)return 10;
    if(a==8&&b==20)return 11;
    if(a==13&&b==4)return 14;
    if(a==13&&b==5)return 15;
    if(a==13&&b==20)return 16;
    if(a==18&&b==20)return 19;
    return -1;
}
int baseVowel(int v){
    switch(v){case 9:case 10:case 11:return 8; case 14:case 15:case 16:return 13; case 19:return 18; default:return -1;}
}

bool koreanKey(SDL_Keycode key,bool shift,int& l,int& v){
    l=v=-1;
    switch(key){
        case SDLK_r:l=shift?1:0;return true; case SDLK_s:l=2;return true; case SDLK_e:l=shift?4:3;return true;
        case SDLK_f:l=5;return true; case SDLK_a:l=6;return true; case SDLK_q:l=shift?8:7;return true;
        case SDLK_t:l=shift?10:9;return true; case SDLK_d:l=11;return true; case SDLK_w:l=shift?13:12;return true;
        case SDLK_c:l=14;return true; case SDLK_z:l=15;return true; case SDLK_x:l=16;return true; case SDLK_v:l=17;return true; case SDLK_g:l=18;return true;
        case SDLK_k:v=0;return true; case SDLK_o:v=shift?3:1;return true; case SDLK_i:v=2;return true; case SDLK_j:v=4;return true;
        case SDLK_p:v=shift?7:5;return true; case SDLK_u:v=6;return true; case SDLK_h:v=8;return true; case SDLK_y:v=12;return true;
        case SDLK_n:v=13;return true; case SDLK_b:v=17;return true; case SDLK_m:v=18;return true; case SDLK_l:v=20;return true;
        default:return false;
    }
}
}


FrontendSystem::FrontendSystem()=default;

void FrontendSystem::setRoster(const std::array<bool,4>& occupied,const std::array<std::string,4>& names){
    occupied_=occupied; rosterNames_=names;
}

void FrontendSystem::beginGeneration(){
    stats_={{10,10,10,10,10,10}};
    pointsLeft_=30;
    selectedStat_=0;
    race_=sex_=klass_=0;
    name_.clear();
    nameInputMode_=NameInputMode::Korean;
    clearHangulComposition();
    notice_.clear();
    screen_=FrontendScreen::Attributes;
}

void FrontendSystem::forceScreen(const std::string& name){
    introStageStart_=0;
    if(name=="copyright") screen_=FrontendScreen::IntroCopyright;
    else if(name=="presents") screen_=FrontendScreen::IntroPresents;
    else if(name=="release") screen_=FrontendScreen::IntroRelease;
    else if(name=="masterpiece") screen_=FrontendScreen::IntroMasterpiece;
    else if(name=="credits") screen_=FrontendScreen::IntroCredits;
    else if(name=="title") screen_=FrontendScreen::Title;
    else if(name=="menu") screen_=FrontendScreen::Menu;
    else if(name=="replacement") screen_=FrontendScreen::Replacement;
    else if(name=="selection") screen_=FrontendScreen::Selection;
    else if(name=="loading") screen_=FrontendScreen::Loading;
    else if(name=="insert") screen_=FrontendScreen::InsertDisk;
    else if(name=="attributes"){ beginGeneration(); }
    else if(name=="race"){ beginGeneration(); pointsLeft_=0; screen_=FrontendScreen::Race; }
    else if(name=="sex"){ beginGeneration(); pointsLeft_=0; applyRace(0); screen_=FrontendScreen::Sex; }
    else if(name=="class"){ beginGeneration(); pointsLeft_=0; applyRace(0); sex_=0; screen_=FrontendScreen::Class; }
    else if(name=="name"){ beginGeneration(); pointsLeft_=0; applyRace(0); applyClass(0); screen_=FrontendScreen::Name; }
    else if(name=="confirm"){ beginGeneration(); pointsLeft_=0; applyRace(0); applyClass(0); name_="Avatar"; screen_=FrontendScreen::Confirm; }
}

const char* FrontendSystem::raceName(int v){
    static constexpr const char* N[4]={"Human","Elf","Dwarf","Bobbit"};
    return N[std::clamp(v,0,3)];
}
const char* FrontendSystem::sexName(int v){ return v?"Female":"Male"; }
const char* FrontendSystem::className(int v){
    static constexpr const char* N[4]={"Fighter","Cleric","Wizard","Thief"};
    return N[std::clamp(v,0,3)];
}

void FrontendSystem::applyRace(int c){
    race_=std::clamp(c,0,3);
    // GEN.EXE 0xADB..0xB4F, DS player buffer A30:
    // Human +5 INT, Elf +5 AGI, Dwarf +5 STR, Bobbit +10 WIS -5 STR.
    if(race_==0) stats_[5]+=5;
    else if(race_==1) stats_[1]+=5;
    else if(race_==2) stats_[0]+=5;
    else { stats_[4]+=10; stats_[0]-=5; }
}
void FrontendSystem::applyClass(int c){
    klass_=std::clamp(c,0,3);
    // GEN.EXE 0xD72..0xDE6: Fighter +10 STR/+10 AGI,
    // Cleric +10 WIS, Wizard +10 INT, Thief +10 AGI.
    if(klass_==0){ stats_[0]+=10; stats_[1]+=10; }
    else if(klass_==1) stats_[4]+=10;
    else if(klass_==2) stats_[5]+=10;
    else stats_[1]+=10;
}

PlayerSave FrontendSystem::buildPlayer() const{
    PlayerSave p{};
    p.name=name_; p.race=static_cast<std::uint16_t>(race_); p.sex=static_cast<std::uint16_t>(sex_); p.klass=static_cast<std::uint16_t>(klass_);
    p.hits=150;
    p.strength=static_cast<std::uint16_t>(stats_[0]); p.agility=static_cast<std::uint16_t>(stats_[1]);
    p.stamina=static_cast<std::uint16_t>(stats_[2]); p.charisma=static_cast<std::uint16_t>(stats_[3]);
    p.wisdom=static_cast<std::uint16_t>(stats_[4]); p.intelligence=static_cast<std::uint16_t>(stats_[5]);
    p.coin=100; p.experience=0; p.food=200;
    p.readyWeapon=1; p.readySpell=0; p.readyArmour=1; p.transport=0;

    // GEN.EXE file 0x138D..0x139A calls 0x40C4 and stores
    //   PLAYER+0x32 = gen_time_seed + Wisdom + Class.
    // 0x40C4 uses DOS int 21h/AH=2Ch (CH hour, CL minute, DH second,
    // DL hundredths), shifts CH*2 / CL*4 / DH*8 as 8-bit registers,
    // adds CX+DX, XORs 0x91EB and masks to 12 bits.  Keep this
    // separate from SDL_GetTicks(): the old M28 approximation could create
    // save seeds that GEN.EXE can never emit and, for some of those seeds,
    // a SPACE universe with no hostile ships at all.
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
    const std::uint16_t genTimeSeed=static_cast<std::uint16_t>(((cx+dx)^0x91EBu)&0x0FFFu);
    p.rngSeedWord=static_cast<std::uint16_t>(genTimeSeed + p.wisdom + p.klass);
    p.x=49; p.y=40;
    p.armour[0]=0xffff; p.armour[1]=1;
    p.weapons[0]=0xffff; p.weapons[1]=2;
    p.spells[0]=0xffff;
    p.transports[0]=0xffff;
    p.worldObjects[0].w={{10,1,49,40,0,0,0,0}};
    return p;
}

namespace {
constexpr std::uint32_t dosTicksToMs(std::uint32_t ticks){
    // ULTIMA.EXE delay helper hooks DOS INT 1Ch.  One BIOS timer tick is
    // 65536 / 1193182 seconds (~54.9254 ms).  Round upward because the DOS
    // loop waits until the requested number of full ticks has elapsed.
    return static_cast<std::uint32_t>((static_cast<std::uint64_t>(ticks)*65536u*1000u + 1193181u)/1193182u);
}

void drawTitleFlag(SDL_Renderer* r,const OriginalData& original,int frame,int sx){
    const auto& mask=original.titleFlagFrame(frame);
    // ULTIMA.EXE 0x5E9 uses logical EGA 10 for mask pixels and 11 for cleared
    // pixels while the title crossover palette is active.  That maps to
    // physical 13 (light magenta) and 11 (light cyan), respectively.
    constexpr SDL_Color fg{255,85,255,255};
    constexpr SDL_Color bg{85,255,255,255};
    for(int y=0;y<7;++y){
        for(int x=0;x<7;++x){
            const auto c=mask[static_cast<std::size_t>(y*8+x)]?fg:bg;
            SDL_SetRenderDrawColor(r,c.r,c.g,c.b,c.a);
            SDL_Rect q{(sx+x)*2,(55+y)*2,2,2};
            SDL_RenderFillRect(r,&q);
        }
    }
}
}

void FrontendSystem::update(std::uint32_t now){
    auto begin=[&](FrontendScreen next){
        screen_=next; introStageStart_=now;
        if(next==FrontendScreen::Title){
            titleFlagStep_=0;
            // CASTLE.16 already contains the middle-looking static flag pose;
            // the original performs the first dynamic redraw after delay(3).
            titleFlagLeft_=1; titleFlagRight_=1;
        }
    };

    // ULTIMA.EXE draws the copyright card before it loads CASTLE.16.  On DOS
    // the card remains visible while that synchronous disk I/O is happening.
    // A modern SDL build preloads the data, so advancing after one host frame
    // made the original first screen effectively invisible.  Preserve it as a
    // real startup stage for 4.0 seconds, then enter the attract loop.  This is
    // an explicit Korean-port presentation choice requested for readability;
    // the card geometry and contents remain oracle-backed.
    if(screen_==FrontendScreen::IntroCopyright){
        if(!introStageStart_){ introStageStart_=now; return; }
        if(now-introStageStart_ < 4000u) return;
        begin(FrontendScreen::IntroPresents);
        return;
    }

    if(screen_==FrontendScreen::IntroPresents){
        if(!introStageStart_){ introStageStart_=now; return; }
        const auto elapsed=now-introStageStart_;
        // F4C first reveals the ORIGIN mask through ABF/828, which polls the
        // keyboard during the CPU-bound dissolve.  Only after the reveal does
        // FB3 wait 5 ticks, print Presents..., and hold for 45 ticks.
        if(elapsed < LOGO_DISSOLVE_MS){
            if(introSkipPending_){ introSkipPending_=false; begin(FrontendScreen::IntroCredits); }
            return;
        }
        const auto presentAt=LOGO_DISSOLVE_MS+dosTicksToMs(5);
        const auto doneAt=presentAt+dosTicksToMs(45);
        if(elapsed < doneAt) return;
        if(introSkipPending_){ introSkipPending_=false; begin(FrontendScreen::IntroCredits); }
        else begin(FrontendScreen::IntroRelease);
        return;
    }

    if(screen_==FrontendScreen::IntroRelease){
        if(!introStageStart_){ introStageStart_=now; return; }
        if(introSkipPending_){ introSkipPending_=false; begin(FrontendScreen::IntroCredits); return; }
        if(now-introStageStart_ < dosTicksToMs(75)) return; // E08 first card: 75 x delay(1)
        begin(FrontendScreen::IntroMasterpiece);
        return;
    }

    if(screen_==FrontendScreen::IntroMasterpiece){
        if(!introStageStart_){ introStageStart_=now; return; }
        if(introSkipPending_){ introSkipPending_=false; begin(FrontendScreen::IntroCredits); return; }
        if(now-introStageStart_ < dosTicksToMs(75)) return; // E08 second card
        begin(FrontendScreen::Title);
        return;
    }

    if(screen_==FrontendScreen::Title){
        if(!introStageStart_){ introStageStart_=now; return; }
        if(introSkipPending_){ introSkipPending_=false; begin(FrontendScreen::IntroCredits); return; }
        // D65 performs 100 iterations of delay(3), and after each delay calls
        // 0x673 twice: once for x=123 and once for x=196.  Each call chooses
        // independently among the three flag masks.  Advance by completed
        // 3-tick quanta so SDL frame rate does not change animation speed.
        const auto elapsed=now-introStageStart_;
        const auto stepMs=dosTicksToMs(3);
        const std::uint32_t wanted=std::min<std::uint32_t>(100u, stepMs?elapsed/stepMs:0u);
        while(titleFlagStep_<wanted){
            auto pick=[&](){
                titleFlagRng_=titleFlagRng_*1664525u+1013904223u;
                return static_cast<int>((titleFlagRng_>>16)%3u);
            };
            titleFlagLeft_=pick();
            titleFlagRight_=pick();
            ++titleFlagStep_;
        }
        if(elapsed < dosTicksToMs(300)) return; // D65: 100 x delay(3)
        begin(FrontendScreen::IntroPresents);   // attract loop repeats
        return;
    }

    if(screen_==FrontendScreen::IntroCredits){
        if(!introStageStart_){ introStageStart_=now; return; }
        // C27 begins by calling B26.  B26 repaints one row from the top and
        // one from the bottom on each iteration until the two halves meet.
        // The original has no timer here; its speed is CPU/adapter dependent,
        // so SDL uses a visible host-duration adaptation before delay(60).
        if(now-introStageStart_ < LOGO_INWARD_MS+dosTicksToMs(60)) return;
        introStageStart_=0;
        screen_=FrontendScreen::Menu;
    }
}

FrontendAction FrontendSystem::handleKey(SDL_Keycode k,SDL_Keymod mod,PlayerSave& out,bool haveSave){
    if(screen_==FrontendScreen::IntroCopyright || screen_==FrontendScreen::IntroPresents){
        // During the copyright/load and ORIGIN delay the original launcher is
        // not polling the keyboard.  DOS still buffers the keystroke; it is
        // consumed as soon as the first polling stage begins after delay(45).
        introSkipPending_=true;
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::IntroRelease || screen_==FrontendScreen::IntroMasterpiece || screen_==FrontendScreen::Title){
        // Marketing cards poll once per delay(1); the title polls after each
        // delay(3).  The SDL event is latched and update() takes the same exit
        // path to the 60-tick conversion-credit card.
        introSkipPending_=true;
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::IntroCredits){
        // C27 does not poll during its fixed delay(60); ignore buffered input.
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Menu){
        // HF74: ESC at the start menu must never terminate the executable.
        // ESC in character generation/selection still returns to this menu.
        if(k==SDLK_ESCAPE) return FrontendAction::None;
        if(k==SDLK_q) return FrontendAction::Quit;
        if(k==SDLK_a){
            int free=-1;
            for(int i=0;i<4;++i) if(!occupied_[static_cast<std::size_t>(i)]){ free=i; break; }
            if(free<0){ screen_=FrontendScreen::Replacement; notice_.clear(); }
            else { generationSlot_=free; beginGeneration(); }
            return FrontendAction::None;
        }
        if(k==SDLK_b){ screen_=FrontendScreen::Selection; notice_.clear(); return FrontendAction::None; }
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Replacement){
        if(k==SDLK_ESCAPE){ screen_=FrontendScreen::Menu; return FrontendAction::None; }
        if(k>='1' && k<='4'){ generationSlot_=static_cast<int>(k-'1'); beginGeneration(); }
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Selection){
        const bool any=occupied_[0]||occupied_[1]||occupied_[2]||occupied_[3];
        if(k==SDLK_ESCAPE){ screen_=FrontendScreen::Menu; return FrontendAction::None; }
        if(!any){
            if(k==SDLK_RETURN || k==SDLK_KP_ENTER){ generationSlot_=0; beginGeneration(); }
            return FrontendAction::None;
        }
        if(k>='1' && k<='4'){
            const int slot=static_cast<int>(k-'1');
            if(occupied_[static_cast<std::size_t>(slot)]){
                selectedSlot_=slot;
                screen_=FrontendScreen::Loading;
                enterGamePending_=true;
                return FrontendAction::StartGame;
            }
        }
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Loading) return FrontendAction::None;
    if(screen_==FrontendScreen::InsertDisk){
        if(k==SDLK_SPACE) screen_=FrontendScreen::Loading;
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Attributes){
        if(k==SDLK_ESCAPE){ screen_=FrontendScreen::Menu; return FrontendAction::None; }
        if(k==SDLK_UP){ selectedStat_=(selectedStat_+5)%6; }
        else if(k==SDLK_DOWN){ selectedStat_=(selectedStat_+1)%6; }
        else if(k==SDLK_LEFT && stats_[selectedStat_]>10){ --stats_[selectedStat_]; ++pointsLeft_; }
        else if(k==SDLK_RIGHT && stats_[selectedStat_]<25 && pointsLeft_>0){ ++stats_[selectedStat_]; --pointsLeft_; }
        else if(k==SDLK_SPACE && pointsLeft_==0){ screen_=FrontendScreen::Race; }
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Race){
        if(k>=SDLK_a && k<=SDLK_d){ applyRace(static_cast<int>(k-SDLK_a)); screen_=FrontendScreen::Sex; }
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Sex){
        if(k==SDLK_a || k==SDLK_b){ sex_=(k==SDLK_b)?1:0; screen_=FrontendScreen::Class; }
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Class){
        if(k>=SDLK_a && k<=SDLK_d){ applyClass(static_cast<int>(k-SDLK_a)); screen_=FrontendScreen::Name; }
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Name){
        const bool shift=(mod & KMOD_SHIFT)!=0;
        if(k==SDLK_SPACE && shift){ toggleNameInputMode(); return FrontendAction::None; }
        if(k==SDLK_BACKSPACE){ backspaceName(); return FrontendAction::None; }
        if(k==SDLK_RETURN || k==SDLK_KP_ENTER){
            commitHangulComposition();
            if(!name_.empty()) screen_=FrontendScreen::Confirm;
            return FrontendAction::None;
        }
        if(nameInputMode_==NameInputMode::Korean) inputKoreanKey(k,mod);
        else inputEnglishKey(k,mod);
        return FrontendAction::None;
    }
    if(screen_==FrontendScreen::Confirm){
        if(k==SDLK_y){
            out=buildPlayer();
            selectedSlot_=generationSlot_;
            screen_=FrontendScreen::Menu;
            return FrontendAction::SaveCharacter;
        }
        if(k==SDLK_n || k==SDLK_ESCAPE){ screen_=FrontendScreen::Menu; }
    }
    return FrontendAction::None;
}


std::size_t FrontendSystem::utf8Length(std::string_view s){
    std::size_t n=0;
    for(std::size_t i=0;i<s.size();++n){
        const unsigned char c=static_cast<unsigned char>(s[i]);
        if(c<0x80) ++i; else if((c&0xE0)==0xC0 && i+1<s.size()) i+=2; else if((c&0xF0)==0xE0 && i+2<s.size()) i+=3; else ++i;
    }
    return n;
}

void FrontendSystem::clearHangulComposition(){ hangulL_=-1; hangulV_=-1; hangulT_=0; }

std::string FrontendSystem::displayName() const{
    std::string out=name_;
    if(hangulL_>=0 && hangulV_>=0) appendUtf8(out,syllable(hangulL_,hangulV_,hangulT_));
    else if(hangulL_>=0) appendUtf8(out,compatL(hangulL_));
    else if(hangulV_>=0) appendUtf8(out,compatV(hangulV_));
    return out;
}

void FrontendSystem::commitHangulComposition(){
    if(hangulL_<0 && hangulV_<0) return;
    if(utf8Length(name_)>=14){ clearHangulComposition(); return; }
    if(hangulL_>=0 && hangulV_>=0) appendUtf8(name_,syllable(hangulL_,hangulV_,hangulT_));
    else if(hangulL_>=0) appendUtf8(name_,compatL(hangulL_));
    else appendUtf8(name_,compatV(hangulV_));
    clearHangulComposition();
}

void FrontendSystem::toggleNameInputMode(){
    commitHangulComposition();
    nameInputMode_=(nameInputMode_==NameInputMode::Korean)?NameInputMode::English:NameInputMode::Korean;
}

void FrontendSystem::backspaceName(){
    if(hangulT_){
        int first=0,next=-1;
        if(splitFinal(hangulT_,first,next)) hangulT_=first; else hangulT_=0;
        return;
    }
    if(hangulV_>=0){ const int b=baseVowel(hangulV_); if(b>=0) hangulV_=b; else hangulV_=-1; return; }
    if(hangulL_>=0){ hangulL_=-1; return; }
    if(name_.empty()) return;
    std::size_t p=name_.size()-1;
    while(p>0 && (static_cast<unsigned char>(name_[p])&0xC0)==0x80) --p;
    name_.erase(p);
}

void FrontendSystem::inputEnglishKey(SDL_Keycode key,SDL_Keymod mod){
    if(utf8Length(name_)>=14) return;
    const bool shift=(mod & KMOD_SHIFT)!=0;
    const bool caps=(mod & KMOD_CAPS)!=0;
    char ch=0;
    if(key>=SDLK_a && key<=SDLK_z){ ch=static_cast<char>('a'+(key-SDLK_a)); if(shift^caps) ch=static_cast<char>(std::toupper(static_cast<unsigned char>(ch))); }
    else if(key>=SDLK_0 && key<=SDLK_9) ch=static_cast<char>('0'+(key-SDLK_0));
    else if(key==SDLK_SPACE) ch=' ';
    else if(key==SDLK_MINUS) ch=shift?'_':'-';
    else if(key==SDLK_PERIOD) ch='.';
    else if(key==SDLK_QUOTE) ch='\'';
    if(ch) name_.push_back(ch);
}

void FrontendSystem::inputKoreanKey(SDL_Keycode key,SDL_Keymod mod){
    const bool shift=(mod & KMOD_SHIFT)!=0;
    int l=-1,v=-1;
    if(!koreanKey(key,shift,l,v)){
        if(key==SDLK_SPACE){ commitHangulComposition(); if(utf8Length(name_)<14) name_.push_back(' '); }
        else if(key>=SDLK_0 && key<=SDLK_9){ commitHangulComposition(); if(utf8Length(name_)<14) name_.push_back(static_cast<char>('0'+(key-SDLK_0))); }
        return;
    }
    if(utf8Length(displayName())>=14 && hangulL_<0 && hangulV_<0) return;
    if(l>=0){
        if(hangulL_<0 && hangulV_<0){ hangulL_=l; return; }
        if(hangulL_<0 && hangulV_>=0){ commitHangulComposition(); hangulL_=l; return; }
        if(hangulL_>=0 && hangulV_<0){ commitHangulComposition(); hangulL_=l; return; }
        if(hangulT_==0){ const int t=finalForL(l); if(t){ hangulT_=t; return; } commitHangulComposition(); hangulL_=l; return; }
        const int t=finalForL(l); const int combo=t?combineFinal(hangulT_,t):0;
        if(combo){ hangulT_=combo; return; }
        commitHangulComposition(); hangulL_=l; return;
    }
    // vowel
    if(hangulL_<0 && hangulV_<0){ hangulV_=v; return; }
    if(hangulL_<0 && hangulV_>=0){ const int c=combineVowel(hangulV_,v); if(c>=0){hangulV_=c;return;} commitHangulComposition(); hangulV_=v; return; }
    if(hangulL_>=0 && hangulV_<0){ hangulV_=v; return; }
    if(hangulT_==0){ const int c=combineVowel(hangulV_,v); if(c>=0){hangulV_=c;return;} commitHangulComposition(); hangulV_=v; return; }
    int first=0,nextL=-1;
    if(splitFinal(hangulT_,first,nextL)){ hangulT_=first; commitHangulComposition(); hangulL_=nextL; hangulV_=v; return; }
    nextL=lForFinal(hangulT_); hangulT_=0; commitHangulComposition(); hangulL_=nextL; hangulV_=v;
}

void FrontendSystem::draw(SDL_Renderer* r,const PixelFont& font,const OriginalData& original,bool haveSave) const{
    SDL_SetRenderDrawColor(r,0,0,0,255); SDL_RenderClear(r);
    if(screen_==FrontendScreen::IntroCopyright){
        // 1987 DOS ULTIMA.EXE opening copyright card (WinWorld captures agree
        // with the strings embedded at raw 0x434E..0x4369).
        // ULTIMA.EXE routine at raw 0x73B.  The BIOS cursor positions are
        // x=16,y=8 for the title, x=8,y=11 for the subtitle, and x=0,y=21
        // for the copyright.  The two blue rules are six Bresenham calls:
        // x=112..200 inclusive at y=58..60 and y=74..76.
        // Korean localization: title/subtitle use the mixed-width front-end font
        // (ASCII 8x16, Hangul 16x16) and are centered by their actual rendered width.
        // Keep the original visual idea of the three-pixel blue rules, but size them
        // from the localized title rather than from the old English literal.
        const std::string titleKo = u8"울티마 I";
        const std::string subtitleKo = u8"제1차 암흑기";
        const int titleW = font.measureFrontend(titleKo);
        const int subtitleW = font.measureFrontend(subtitleKo);
        const int titleX = (640-titleW)/2;
        const int subtitleX = (640-subtitleW)/2;
        const int rulePad = 25; // preserve the original rule's extra breathing room
        const int ruleX0 = std::max(0,titleX-rulePad);
        const int ruleX1 = std::min(639,titleX+titleW+rulePad-1);
        font.drawFrontend(r,titleX,128,titleKo,WHITE);
        SDL_SetRenderDrawColor(r,FRONT_BLUE.r,FRONT_BLUE.g,FRONT_BLUE.b,FRONT_BLUE.a);
        SDL_Rect upperRule{ruleX0,116,ruleX1-ruleX0+1,6};
        SDL_Rect lowerRule{ruleX0,148,ruleX1-ruleX0+1,6};
        SDL_RenderFillRect(r,&upperRule);
        SDL_RenderFillRect(r,&lowerRule);
        font.drawFrontend(r,subtitleX,176,subtitleKo,WHITE);
        // M68: render the copyright line with the mixed-width frontend font.
        // ASCII glyphs are 8 px wide x 16 px tall in host pixels, instead of
        // the 16x16 host-scaled BIOS glyphs, and center the line by its actual width.
        centeredFrontText(r,font,21,"Copyright (C) 1987 Origin Systems, Inc.",WHITE);
        return;
    }
    if(screen_==FrontendScreen::IntroPresents || screen_==FrontendScreen::IntroRelease ||
       screen_==FrontendScreen::IntroMasterpiece){
        const auto now=SDL_GetTicks();
        const auto elapsed=introStageStart_?now-introStageStart_:0u;
        if(screen_==FrontendScreen::IntroPresents && elapsed<LOGO_DISSOLVE_MS)
            drawLauncherLogoDissolve(r,original,elapsed);
        else
            drawLauncherLogo(r,original);
        if(screen_==FrontendScreen::IntroPresents){
            // F4C/FB3 timing remains oracle-backed.  M64 localizes the visible
            // launcher caption and centers it by its actual Korean render width.
            if(elapsed>=LOGO_DISSOLVE_MS+dosTicksToMs(5))
                centeredFrontText(r,font,13,IntroKoStrings::presents,FRONT_CYAN);
        } else if(screen_==FrontendScreen::IntroRelease){
            // Preserve the original E08 visual indentation: the second visible
            // line starts three ASCII columns (the width of "...") to the right.
            centeredIndentedFrontPair(r,font,12,IntroKoStrings::release1,IntroKoStrings::release2,FRONT_CYAN);
        } else {
            // Preserve the same three-column indentation on the masterpiece card.
            centeredIndentedFrontPair(r,font,12,IntroKoStrings::masterpiece1,IntroKoStrings::masterpiece2,FRONT_CYAN);
        }
        return;
    }
    if(screen_==FrontendScreen::IntroCredits){
        // C27 calls B26 first.  B26 redraws source rows 20/83, 21/82, ...
        // until both halves meet in the middle.  Legal text appears only after
        // that logo redraw has completed.
        const auto now=SDL_GetTicks();
        const auto elapsed=introStageStart_?now-introStageStart_:0u;
        if(elapsed<LOGO_INWARD_MS){
            drawLauncherLogoInward(r,original,elapsed);
            return;
        }
        drawLauncherLogo(r,original);
        // M71 shifts the entire localized credit block up by one row while
        // preserving the established spacing between trademark, conversion,
        // and localization lines.
        centeredFrontText(r,font,16,IntroKoStrings::trademark1,WHITE);
        centeredFrontText(r,font,17,IntroKoStrings::trademark2,WHITE);
        centeredFrontText(r,font,18,IntroKoStrings::trademark3,WHITE);

        // M75 shifts both credit lines five ASCII spaces (40 px in the logical
        // 640x400 canvas) to the left as one block, while preserving the exact
        // shared colon x-coordinate established in M72.
        const std::string conversion=IntroKoStrings::conversion;
        const int conversionW=font.measureFrontend(conversion);
        constexpr int creditShiftLeft=5*8;
        const int conversionX=std::max(0,(640-conversionW)/2-creditShiftLeft);
        font.drawFrontend(r,conversionX,21*16,conversion,WHITE);
        const int creditColonX=conversionX+font.measureFrontend("컨버전");
        const int localizationX=creditColonX-font.measureFrontend("한글화");
        font.drawFrontend(r,localizationX,22*16,IntroKoStrings::localization,WHITE);
        return;
    }
    if(screen_==FrontendScreen::Title){
        if(auto* t=original.titleTexture()){
            SDL_Rect dst{0,0,640,400}; SDL_RenderCopy(r,t,nullptr,&dst);
            // Before the first delay(3), DOS shows the two poses baked into
            // CASTLE.16.  Dynamic repaint starts only after the first loop.
            if(titleFlagStep_>0){
                drawTitleFlag(r,original,titleFlagLeft_,123);
                drawTitleFlag(r,original,titleFlagRight_,196);
            }
        }
        return;
    }
    if(screen_==FrontendScreen::Menu){
        drawGenFrame(r);
        centeredFrontText(r,font,6,u8"*** 울티마 I ***",WHITE);
        centeredFrontText(r,font,9,GenKoStrings::menuDarkestDungeon,WHITE);
        centeredFrontText(r,font,10,GenKoStrings::menuFarthestSpace,WHITE);
        centeredFrontText(r,font,14,GenKoStrings::generate,FRONT_CYAN);
        centeredFrontText(r,font,15,GenKoStrings::cont,FRONT_CYAN);

        // Center the localized prompt and animated cursor as one visual group.
        // M63 uses the same rule as the character-generation prompts: the
        // cursor begins exactly two 8 px ASCII spaces after the colon.
        const int choiceW=font.measureFrontend(GenKoStrings::choice);
        constexpr int choiceCursorGap=2*8;
        constexpr int cursorW=16;
        const int choiceX=(640-(choiceW+choiceCursorGap+cursorW))/2;
        font.drawFrontend(r,choiceX,18*16,GenKoStrings::choice,FRONT_CYAN);
        drawInputCursorPx(r,original,choiceX+choiceW+choiceCursorGap,18*16,FRONT_CYAN);
        if(!notice_.empty()) centeredFrontText(r,font,20,notice_,YELLOW);
        return;
    }
    if(screen_==FrontendScreen::Replacement){
        drawGenFrame(r);
        drawReplacementTitle(r,font);
        for(int i=0;i<4;++i){
            const std::string row=std::to_string(i+1)+") "+rosterNames_[static_cast<std::size_t>(i)];
            frontText(r,font,15,6+i,row,FRONT_CYAN);
        }

        // Localized Character Replacement instructions.  The two instruction
        // lines are centered by their actual rendered widths.  As in the
        // original GEN screen, the animated input cursor sits immediately
        // after the final "..." on the last instruction line.
        centeredFrontText(r,font,14,GenKoStrings::replacementHelp1,FRONT_CYAN);
        const int replacementHelp2W=font.measureFrontend(GenKoStrings::replacementHelp2);
        const int replacementHelp2X=std::max(0,(640-replacementHelp2W)/2);
        font.drawFrontend(r,replacementHelp2X,15*16,GenKoStrings::replacementHelp2,FRONT_CYAN);
        drawInputCursorPx(r,original,replacementHelp2X+replacementHelp2W,15*16,FRONT_CYAN);
        return;
    }
    if(screen_==FrontendScreen::Selection){
        drawGenFrame(r);
        drawSelectionTitle(r,font);
        const bool any=occupied_[0]||occupied_[1]||occupied_[2]||occupied_[3];
        if(!any){
            // Localized empty-roster message.  Keep the selection title/frame
            // identical to the populated-roster path, center every text row,
            // and place the animated cursor immediately after the final "...".
            centeredFrontText(r,font,10,GenKoStrings::selectionEmpty1,FRONT_CYAN);
            centeredFrontText(r,font,11,GenKoStrings::selectionEmpty2,FRONT_CYAN);
            const int selectionEmpty3W=font.measureFrontend(GenKoStrings::selectionEmpty3);
            const int selectionEmpty3X=std::max(0,(640-selectionEmpty3W)/2);
            font.drawFrontend(r,selectionEmpty3X,12*16,GenKoStrings::selectionEmpty3,FRONT_CYAN);
            drawInputCursorPx(r,original,selectionEmpty3X+selectionEmpty3W,12*16,FRONT_CYAN);
        } else {
            for(int i=0;i<4;++i){
                const std::string row=std::to_string(i+1)+") "+rosterNames_[static_cast<std::size_t>(i)];
                frontText(r,font,15,6+i,row,FRONT_CYAN);
            }

            // Localized Character Selection instructions.  Each text line is
            // centered by its own rendered width.  GEN.EXE leaves the keyboard
            // input cursor at the current text position after the final "...",
            // so the animated cursor begins immediately after the ellipsis.
            centeredFrontText(r,font,14,GenKoStrings::selectionHelp1,FRONT_CYAN);
            const int selectionHelp2W=font.measureFrontend(GenKoStrings::selectionHelp2);
            const int selectionHelp2X=std::max(0,(640-selectionHelp2W)/2);
            font.drawFrontend(r,selectionHelp2X,15*16,GenKoStrings::selectionHelp2,FRONT_CYAN);
            drawInputCursorPx(r,original,selectionHelp2X+selectionHelp2W,15*16,FRONT_CYAN);
        }
        return;
    }

    if(screen_==FrontendScreen::Loading || screen_==FrontendScreen::InsertDisk){
        drawLoadingFrame(r);
        // M76 localizes GEN.EXE 0x1843's two loading rows.  Preserve the
        // original row positions (8 and 10), but center each Korean line by
        // its actual mixed-width frontend render width.
        centeredFrontText(r,font,8,GenKoStrings::loading1,WHITE);
        centeredFrontText(r,font,10,GenKoStrings::loading2,WHITE);
        if(screen_==FrontendScreen::InsertDisk){
            // GEN.EXE 0x17B3: disk-change prompt at cells (1,23) and (1,24), cyan.
            frontText(r,font,1,23,"울티마 I 디스켓을 넣으십시오",FRONT_CYAN);
            frontText(r,font,1,24,"스페이스바를 눌러 계속하세요...",FRONT_CYAN);
        }
        return;
    }

    drawGenFrame(r);
    drawGenerationTitle(r,font);

    if(screen_==FrontendScreen::Attributes){
        const std::string pointsLine=std::string(GenKoStrings::points)+"  "+std::to_string(pointsLeft_);
        centeredFrontText(r,font,4,pointsLine,FRONT_CYAN);
        const StatsLayout sl=drawStats(r,font,stats_);
        const int cy=(6+selectedStat_)*16;
        drawStatCursor(r,sl.x-16,cy,true);
        drawStatCursor(r,sl.x+sl.width,cy,false);
        centeredFrontText(r,font,16,GenKoStrings::help1,FRONT_CYAN);
        centeredFrontText(r,font,17,GenKoStrings::help2,FRONT_CYAN);
        centeredFrontText(r,font,18,GenKoStrings::help3,FRONT_CYAN);
        return;
    }

    // From the race step onward GEN.EXE preserves the six stat rows and only
    // clears/rewrites the lower prompt region. Render the same persistent page.
    const StatsLayout persistentStats=drawStats(r,font,stats_);

    if(screen_==FrontendScreen::Race){
        constexpr int promptX=3*16;
        constexpr int promptCursorGap=2*8; // exactly two ASCII spaces after ':'
        font.drawFrontend(r,promptX,17*16,GenKoStrings::racePrompt,FRONT_CYAN);
        frontText(r,font,12,19,"a) 인간",FRONT_CYAN);
        frontText(r,font,12,20,"b) 엘프",FRONT_CYAN);
        frontText(r,font,12,21,"c) 드워프",FRONT_CYAN);
        frontText(r,font,12,22,"d) 보비트",FRONT_CYAN);
        drawInputCursorPx(r,original,promptX+font.measureFrontend(GenKoStrings::racePrompt)+promptCursorGap,17*16,FRONT_CYAN);
        return;
    }

    constexpr int infoColonX=320;
    auto drawCenteredColonPair=[&](int y,const std::string& label,const std::string& value){
        std::string bareLabel=label;
        if(!bareLabel.empty() && bareLabel.back()==':') bareLabel.pop_back();
        font.drawFrontend(r,infoColonX-font.measureFrontend(bareLabel),y*16,bareLabel,FRONT_CYAN);
        font.drawFrontend(r,infoColonX,y*16,":",FRONT_CYAN);
        font.drawFrontend(r,infoColonX+font.measureFrontend(": "),y*16,value,FRONT_CYAN);
    };

    drawCenteredColonPair(13,GenKoStrings::raceLabel,raceNameKo(race_));
    if(screen_==FrontendScreen::Sex){
        constexpr int promptX=3*16;
        constexpr int promptCursorGap=2*8; // exactly two ASCII spaces after ':'
        font.drawFrontend(r,promptX,17*16,GenKoStrings::sexPrompt,FRONT_CYAN);
        frontText(r,font,12,19,"a) 남성",FRONT_CYAN);
        frontText(r,font,12,20,"b) 여성",FRONT_CYAN);
        drawInputCursorPx(r,original,promptX+font.measureFrontend(GenKoStrings::sexPrompt)+promptCursorGap,17*16,FRONT_CYAN);
        return;
    }

    drawCenteredColonPair(14,GenKoStrings::sexLabel,sexNameKo(sex_));
    if(screen_==FrontendScreen::Class){
        constexpr int promptX=3*16;
        constexpr int promptCursorGap=2*8; // exactly two ASCII spaces after ':'
        font.drawFrontend(r,promptX,17*16,GenKoStrings::classPrompt,FRONT_CYAN);
        frontText(r,font,12,19,"a) 전사",FRONT_CYAN);
        frontText(r,font,12,20,"b) 사제",FRONT_CYAN);
        frontText(r,font,12,21,"c) 마법사",FRONT_CYAN);
        frontText(r,font,12,22,"d) 도적",FRONT_CYAN);
        drawInputCursorPx(r,original,promptX+font.measureFrontend(GenKoStrings::classPrompt)+promptCursorGap,17*16,FRONT_CYAN);
        return;
    }

    drawCenteredColonPair(15,GenKoStrings::classLabel,classNameKo(klass_));
    if(screen_==FrontendScreen::Name){
        const std::string shown=displayName();
        // M50 starts the Korean text-layout layer here.  This wording is not
        // used to reposition the GEN frame; only the localized prompt/cursor
        // are laid out within the existing prompt row.
        constexpr int promptX=3*16;
        constexpr int promptCursorGap=2*8; // exactly two ASCII spaces after ':'
        font.drawFrontend(r,promptX,17*16,GenKoStrings::namePrompt,FRONT_CYAN);
        const int nameX=promptX+font.measureFrontend(GenKoStrings::namePrompt)+promptCursorGap;
        font.drawFrontend(r,nameX,17*16,shown,FRONT_CYAN);
        drawInputCursorPx(r,original,nameX+font.measureFrontend(shown),17*16,FRONT_CYAN);
        drawNameInputModeIndicator(r,font,nameInputMode_==NameInputMode::Korean);
        return;
    }

    if(screen_==FrontendScreen::Confirm){
        font.drawFrontend(r,persistentStats.x,4*16,name_,FRONT_CYAN);
        const std::string prompt=GenKoStrings::savePrompt;
        const int promptX=std::max(0,(640-font.measureFrontend(prompt))/2);
        font.drawFrontend(r,promptX,22*16,prompt,FRONT_CYAN);
        constexpr int cursorGap=8; // keep the confirmation cursor close to the sentence
        drawInputCursorPx(r,original,promptX+font.measureFrontend(prompt)+cursorGap,22*16,FRONT_CYAN);
        return;
    }
}
