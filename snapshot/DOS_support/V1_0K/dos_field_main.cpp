#include "OverworldSimulation.h"
#include "SDL.h"
#include "dos_hangul_table.h"
#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <unistd.h>
#include <pc.h>
#include "dos_pcm_speaker.h"
#include "dos_quest_status.h"
#include "dos_cp949_table.h"
#include <string.h>
extern "C" int dos_native_hangul(void);
extern "C" int dos_video_mode(void);
extern "C" void dos_vesa_frame(void);
extern "C" void dos_ui_rect(int,int,int,int,int);

extern "C" void dos_native_glyph_pixel(int,int);
extern "C" int dos_video_start();
extern "C" void dos_video_end();
extern "C" void dos_video_clear(int);
extern "C" unsigned long dos_draw_writes();
static uint8_t raw[6656],mapdata[13104],pixeldata[256],avatar[256];
static uint8_t latin[4096];
static uint8_t hangul[11520];
static int hangulAvailable=0;
static unsigned hangulGlyphs=0,hangulPixels=0;
static uint8_t saved[820];
static int fontAvailable=0;
static int valueAt(int offset){
 int v=saved[offset]|(saved[offset+1]<<8);
 return v>9999?9999:v;
}
static void textAt(SDL_Renderer*r,int xx,int yy,const char*msg){
 if(!fontAvailable)return;
 // High-resolution UI text must NOT go through the 2x tile renderer.
 // Native VESA glyphs are 8x16 physical pixels, not 16x32.
 if(dos_video_mode()==0x101){
  for(int i=0;msg[i];++i){
   unsigned c=(unsigned char)msg[i];
   for(int y=0;y<16;++y)for(int x=0;x<8;++x)
    if(latin[c*16+y]&(0x80u>>x))
     dos_native_glyph_pixel(xx*2+i*8+x,yy*2+y);
  }
  return;
 }

 SDL_SetRenderDrawColor(r,255,255,255,255);
 for(int i=0;msg[i];++i){
  unsigned c=(unsigned char)msg[i];
  for(int y=0;y<16;++y)for(int x=0;x<8;++x)
   if(latin[c*16+y]&(0x80>>x)) SDL_RenderDrawPoint(r,xx+i*8+x,yy+y);
 }
}
static void dosSavedNameToUtf8(const uint8_t*save,char out[64]);
// 320x200 prototype: half-scale 16x16 Hangul to 8x8, ASCII 8x16 to 4x8.
// This is an interim layout; a future high-resolution DOS driver will use
// the original 16x16 glyphs without discarding source pixels.
static void textKorean(SDL_Renderer*r,int px,int py,const char*utf8){
 if(dos_native_hangul()){
  int originX=px*2,originY=py*2;
  for(const unsigned char*q=(const unsigned char*)utf8;*q;){
   unsigned u=*q++;
   if(u>=0xE0 && u<=0xEF && q[0] && q[1]){
    unsigned ch=((u&15)<<12)|((q[0]&63)<<6)|(q[1]&63);q+=2;
    if(ch>=0xAC00&&ch<=0xD7A3&&hangulAvailable){
     ++hangulGlyphs;
     unsigned n=ch-0xAC00,cho=n/588+1,jung=n/28%21+1,jong=n%28;
     unsigned cs=(jong?kChoWithJong[jung]:kChoNoJong[jung])*20+cho;
     unsigned vs=160+(jong?kJungWithJong[cho]:kJungNoJong[cho])*22+jung;
     unsigned fs=248+(jong?kJongSet[jung]*28+jong:0);
     for(unsigned y=0;y<16;y++){
      unsigned off=y*2;
      unsigned bits=((unsigned)(hangul[cs*32+off]|hangul[vs*32+off]|hangul[fs*32+off])<<8)|
        (hangul[cs*32+off+1]|hangul[vs*32+off+1]|hangul[fs*32+off+1]);
      for(unsigned x=0;x<16;x++)if(bits&(0x8000u>>x)){
       dos_native_glyph_pixel(originX+x,originY+y);++hangulPixels;
      }
     }
    }
    originX+=16;continue;
   }
   if(fontAvailable){
    for(unsigned y=0;y<16;y++)for(unsigned x=0;x<8;x++)
     if(latin[u*16+y]&(0x80u>>x))dos_native_glyph_pixel(originX+x,originY+y);
   }
   originX+=8;
  }
  return;
 }

 for(const unsigned char*q=(const unsigned char*)utf8;*q;){
  unsigned u=*q++;
  if(u>=0xE0 && u<=0xEF && q[0] && q[1]){
   unsigned ch=((u&15)<<12)|((q[0]&63)<<6)|(q[1]&63);q+=2;
   if(ch>=0xAC00 && ch<=0xD7A3 && hangulAvailable){
    ++hangulGlyphs;
    unsigned n=ch-0xAC00,cho=n/588+1,jung=n/28%21+1,jong=n%28;
    unsigned cs=(jong?kChoWithJong[jung]:kChoNoJong[jung])*20+cho;
    unsigned vs=160+(jong?kJungWithJong[cho]:kJungNoJong[cho])*22+jung;
    unsigned fs=248+(jong?kJongSet[jung]*28+jong:0);
    SDL_SetRenderDrawColor(r,255,255,255,255);
    for(unsigned y=0;y<8;y++){
     unsigned off=y*4;
     unsigned bits=((unsigned)(hangul[cs*32+off]|hangul[vs*32+off]|hangul[fs*32+off])<<8)|
       (hangul[cs*32+off+1]|hangul[vs*32+off+1]|hangul[fs*32+off+1]);
     unsigned bits2=((unsigned)(hangul[cs*32+off+2]|hangul[vs*32+off+2]|hangul[fs*32+off+2])<<8)|
       (hangul[cs*32+off+3]|hangul[vs*32+off+3]|hangul[fs*32+off+3]);
     bits|=bits2;
     for(unsigned x=0;x<8;x++)if((bits>>(14-2*x))&3u){
      SDL_RenderDrawPoint(r,px+x,py+y);++hangulPixels;
     }
    }
   }
   px+=8;continue;
  }
  unsigned char c=(unsigned char)u;
  if(fontAvailable){SDL_SetRenderDrawColor(r,255,255,255,255);
   for(int y=0;y<8;y++)for(int x=0;x<4;x++){
    unsigned bits=latin[c*16+y*2]|latin[c*16+y*2+1];
    if(bits&(0xC0u>>(x*2)))SDL_RenderDrawPoint(r,px+x,py+y);
   }
  }
  px+=4;
 }
}
static char commandNotice[128]="";
static unsigned collisionCount=0;
static unsigned terrainSoundEvents=0,monsterBlockedEvents=0,landMoveEvents=0;
static void terrain_collision_sound(){
 if(dosPcmReady[0]){dosPcmPlay(0);return;}
 // Initial DOS PC speaker backend; precise original sample parity remains pending.
 unsigned divisor=1193180u/220u;
 outportb(0x43,0xB6);
 outportb(0x42,divisor&255);
 outportb(0x42,divisor>>8);
 unsigned old= inportb(0x61);
 outportb(0x61,old|3);
 usleep(55000);
 outportb(0x61,old);
}
// Temporary DOS speaker output for sound(4); original waveform parity pending.
static void outdoor_land_sound(){
 if(dosPcmReady[1]){dosPcmPlay(1);return;}
 unsigned old=inportb(0x61);
 unsigned divisor=1193180u/440u;
 outportb(0x43,0xB6);
 outportb(0x42,divisor&255);
 outportb(0x42,divisor>>8);
 outportb(0x61,old|3);
 usleep(17000);
 outportb(0x61,old);
}
static int transport=0;

static int saveValid=0;
static const PlayerSave* livePlayer=nullptr;
static void putWord(int offset,unsigned value){
 saved[offset]=(unsigned char)value;
 saved[offset+1]=(unsigned char)(value>>8);
}
static void sync_status(const PlayerSave& player){
 if(!saveValid)return;
 putWord(0x16,player.hits);
 putWord(0x24,player.coin);
 putWord(0x26,player.experience);
 putWord(0x28,player.food);
}
static void sync_world(const PlayerSave& player){
 if(!saveValid)return;
 for(int i=0;i<40;i++)for(int j=0;j<8;j++){
  int off=0xB4+i*16+j*2;
  unsigned value=player.worldObjects[i].w[j];
  saved[off]=(unsigned char)value;
  saved[off+1]=(unsigned char)(value>>8);
 }
}

static void setPlayerPosition(PlayerSave& p,int x,int y){
 p.x=x;p.y=y;
 p.worldObjects[0].w[2]=(unsigned short)x;
 p.worldObjects[0].w[3]=(unsigned short)y;
}
static int cell(int x,int y);
// Collision checks follow VehicleSystem::mayMove terrain/transport ordering.
static int can_move(int x,int y){
 int c=cell(x,y);
 if(c>=19&&c<=47)return 0;
 if(c==3)return 0;
 if(c==0)return transport==3||transport==4||transport==5;
 if(c==2&&transport==5)return 0;
 if((transport==3||transport==4)&&c!=0)return 0;
 if(transport>=6)return 0;
 return 1;
}
static int terrain(int x,int y){
 x=(x%168+168)%168;y=(y%156+156)%156;
 int off=y*168+x;uint8_t v=mapdata[off/2];return (off&1)?(v&15):(v>>4);
}
int OriginalData::terrainAt(int x,int y) const { return terrain(x,y); }
static int cell(int x,int y){
 x=(x%168+168)%168;y=(y%156+156)%156;
 if(livePlayer){
  for(int i=1;i<40;i++){
   const WorldObject& o=livePlayer->worldObjects[i];
   if(o.tile()&&o.x()==x&&o.y()==y)return o.tile();
  }
 }
 if(saveValid && !livePlayer){
  for(int i=1;i<40;i++){
   int o=0xB4+i*16;
   int tile=saved[o]|(saved[o+1]<<8);
   int ox=saved[o+4]|(saved[o+5]<<8);
   int oy=saved[o+6]|(saved[o+7]<<8);
   if(tile&&ox==x&&oy==y)return tile;
  }
 }
 return terrain(x,y);
}
static void tile_image(int tile,uint8_t*out){
 tile%=52;for(int y=0;y<16;y++)for(int x=0;x<16;x++){
 int v=0;for(int plane=0;plane<4;plane++)if(raw[tile*128+y*8+plane*2+x/8]&(128>>(x%8)))v|=1<<plane;
 out[y*16+x]=(uint8_t)v;
 }
}
static void draw_view(int cx,int cy){
 // HF93: black command/status panes, then game content, then frame on TOP.
 dos_video_clear(0);
 SDL_Renderer*r=nullptr;SDL_SetRenderDrawColor(r,255,255,255,255);
 // Native-resolution independent UI grid, distinct from upscaled 320x200 tiles.
 // Keep within outer/inner frame boundaries.


 for(int dy=-4;dy<=4;dy++)for(int dx=-9;dx<=9;dx++){
 int tile=cell(cx+dx,cy+dy);
 tile_image(tile,pixeldata);
 SDL_Texture tex{16,16,pixeldata};
 SDL_Rect dst{8+(dx+9)*16,8+(dy+4)*16,16,16};
 SDL_RenderCopy(r,&tex,nullptr,&dst);
 }
 if(saveValid){
  char dosName[64];dosSavedNameToUtf8(saved,dosName);
  textKorean(r,45,2,dosName);
 }
 if(saveValid && fontAvailable){
  char line[24];
  sprintf(line,"HP:%u",valueAt(0x16));textAt(r,249,161,line);
  sprintf(line,"FOOD:%u",valueAt(0x28));textAt(r,249,170,line);
  sprintf(line,"XP:%u",valueAt(0x26));textAt(r,249,179,line);
  sprintf(line,"GOLD:%u",valueAt(0x24));textAt(r,249,188,line);
 }
 if(commandNotice[0]){
  // Dedicated opaque notification strip, avoiding text over terrain sprites.
  SDL_SetRenderDrawColor(r,0,0,0,255);
  SDL_Rect strip{1,162,240,16};SDL_RenderFillRect(r,&strip);
  textKorean(r,3,163,commandNotice);
 }
 // Draw the saved player WorldObject[0] physical tile over the terrain.
 int playerTile=saveValid?(saved[0xB4]|(saved[0xB5]<<8)):-1;
 if(playerTile>=0&&playerTile<52){
  tile_image(playerTile,avatar);
  SDL_Texture playerTex{16,16,avatar};
  SDL_Rect playerDst{8+9*16,8+4*16,16,16};
  SDL_RenderCopy(r,&playerTex,nullptr,&playerDst);
 } else {
  SDL_SetRenderDrawColor(r,255,255,255,255);
  SDL_RenderDrawLine(r,160,73,160,87);
  SDL_RenderDrawLine(r,153,80,167,80);
 }
 // Preserve HF93 drawFrames final compositing order and corner geometry.
 dos_vesa_frame();
}

// Interpret the 14-byte original player-name field as CP949, render via the
// existing UTF-8 Hangul glyph compositor. No sidecar or extra save fields.
static void dosSavedNameToUtf8(const uint8_t*save,char out[64]){
 unsigned pos=0;
 for(unsigned i=0;i<14 && save[i] && pos<58;){
  unsigned c=save[i++];
  if(c<0x80){out[pos++]=(char)c;continue;}
  if(i>=14 || !save[i])break;
  unsigned cp=dosCp949Lookup(c,save[i++]);
  if(!cp)continue;
  out[pos++]=(char)(0xE0|(cp>>12));
  out[pos++]=(char)(0x80|((cp>>6)&63));
  out[pos++]=(char)(0x80|(cp&63));
 }
 out[pos]=0;
}
static bool dosCp949Audit(){
 const char*expected=u8"가나다라마바사";
 const unsigned char input[]={0xB0,0xA1,0xB3,0xAA,0xB4,0xD9,0xB6,0xF3,0xB8,0xB6,0xB9,0xD9,0xBB,0xE7,0};
 uint8_t sample[820]={0};
 for(int i=0;i<14;++i)sample[i]=input[i];
 char output[64];dosSavedNameToUtf8(sample,output);
 return strcmp(output,expected)==0;
}
static void dosHelpScreen(){
 dos_video_clear(1);
 SDL_Renderer*r=nullptr;
 textKorean(r,40,22,"커맨드 목록");
 textKorean(r,30,55,"WASD  이동");
 textKorean(r,30,78,"H  도움말");
 textKorean(r,30,101,"J  퀘스트 현황");
 textKorean(r,30,124,"Q  종료");
 textKorean(r,30,159,"스페이스바를 누르세요");
 while(getch()!=' '){}
}
static void dosQuestScreen(const uint8_t*record){
 dos_video_clear(1);
 SDL_Renderer*r=nullptr;
 textKorean(r,80,8,"퀘스트 현황");
 static const char*castle[]={"","로드 브리티시성","잊힌 왕의 성",
  "바라타리아성","론도린성","올림푸스성","흑룡성","백룡성","샤미노성"};
 static const char*goal[]={"","잊힌 영혼의 무덤","젤라틴 큐브",
  "남쪽 표지판","송장 벌레","오지만디아스의 기둥",
  "리치","지식의 탑","발론"};
 static const char*state[]={"","수락","성공","완료"};
 int ids[8], count=0;
 for(int c=1;c<=8;c++)if(dosQuestStatus(record,c)!=DOS_QUEST_HIDDEN)ids[count++]=c;
 for(int page=0;;++page){
  dos_video_clear(1);
  textKorean(r,80,8,"퀘스트 현황");
  if(!count)textKorean(r,25,70,"수락한 퀘스트가 없습니다!");
  for(int row=0;row<4 && page*4+row<count;row++){
   int c=ids[page*4+row],y=27+row*37;
   textKorean(r,8,y,castle[c]);
   textKorean(r,247,y,state[(int)dosQuestStatus(record,c)]);
   textKorean(r,23,y+17,goal[c]);
  }
  textKorean(r,28,181,"스페이스바");
  while(getch()!=' '){}
  if((page+1)*4>=count)break;
 }

}
// CP949 input is accepted as two-byte sequences (for DOS keyboards configured
// to emit CP949); ASCII remains one byte. Reject dangling lead bytes.
static bool dosValidCp949Name(const unsigned char*name,unsigned n){
 if(n==0||n>14)return false;
 for(unsigned i=0;i<n;){
  unsigned c=name[i++];
  if(c<128){if(c<32||c==127)return false;continue;}
  if(i>=n)return false;
  unsigned trail=name[i++];
  if(!dosCp949Lookup(c,trail))return false;
 }
 return true;
}
// Menu coordinates are native 640x400 VESA pixels, with VGA fallback.
static void menuText(SDL_Renderer*r,int x,int y,const char*t){
 if(dos_video_mode()==0x101)textKorean(r,x/2,y/2,t);
 else textKorean(r,x/2,y/2,t);
}
static void menuBorder(){
 if(dos_video_mode()==0x101){
  dos_vesa_frame();
  dos_ui_rect(24,24,592,352,15);
 }
}
static void dosApplyCreationBonuses(int stats[6],int race,int klass){
 if(race==0)stats[5]+=5;
 else if(race==1)stats[1]+=5;
 else if(race==2)stats[0]+=5;
 else{stats[4]+=10;stats[0]-=5;}
 if(klass==0){stats[0]+=10;stats[1]+=10;}
 else if(klass==1)stats[4]+=10;
 else if(klass==2)stats[5]+=10;
 else stats[1]+=10;
}
static bool dosCreationAudit(){
 for(int r=0;r<4;r++)for(int k=0;k<4;k++){
  int a[6]={10,10,10,10,10,40};
  dosApplyCreationBonuses(a,r,k);
  int sum=0;
  for(int i=0;i<6;i++)sum+=a[i];
  if(sum!=90+(r==3?5:5)+(k==0?20:10))return false;
  if(a[0]!=10+(r==2?5:r==3?-5:0)+(k==0?10:0))return false;
  if(a[1]!=10+(r==1?5:0)+(k==0?10:k==3?10:0))return false;
  if(a[4]!=10+(r==3?10:0)+(k==1?10:0))return false;
  if(a[5]!=40+(r==0?5:0)+(k==2?10:0))return false;
 }
 const unsigned char valid[]={'A','v','a','t','a','r'};
 const unsigned char bad[]={0xB0};
 return dosValidCp949Name(valid,6) && !dosValidCp949Name(bad,1);
}
// On-screen 16x16 font specimen: available from field with F.
static void dosFont16Screen(bool awaitKey=true){
 SDL_Renderer*r=nullptr;
 dos_video_clear(1);menuBorder();
 menuText(r,60,34,"한글 16x16 원본 폰트 검사");
 menuText(r,60,90,"가나다라마바사 아자차카타파하");
 menuText(r,60,144,"생명력 식량 경험치 코인");
 menuText(r,60,198,"한글 입력과 메뉴 출력 확인");
 menuText(r,60,252,"ABCDEFG 0123456789");
 menuText(r,60,336,"스페이스바: 돌아가기");
 if(awaitKey)while(getch()!=' '){}
}
static bool dosGenerateCharacter(int slot){
 SDL_Renderer*r=nullptr;
 unsigned char name[15]={0};
 unsigned length=0;
 for(;;){
  dos_video_clear(1);menuBorder();
  menuText(r,92,36,"새 캐릭터 생성");
  menuText(r,36,98,"이름 입력 (CP949 14바이트)");
  char shown[64],line[64];
  uint8_t temp[820]={0};
  for(unsigned i=0;i<length;i++)temp[i]=name[i];
  dosSavedNameToUtf8(temp,shown);
  snprintf(line,sizeof(line),"> %s",shown);
  menuText(r,48,172,line);
  menuText(r,32,280,"Enter 확인  ESC 취소");
  int c=getch();
  if(c==27)return false;
  if(c==8){if(length){--length;if(length && name[length]>=0x80 && name[length-1]>=0x80)--length;name[length]=0;}continue;}
  if(c==13){
   if(!dosValidCp949Name(name,length))continue;
   break;
  }
  if(c==0||c==224){getch();continue;}
  if(length>=14||c<32||c==127)continue;
  name[length++]=(unsigned char)c;
  name[length]=0;
 }
 // GEN.EXE base attributes: six stats start at 10, distribute 30 points.
 int stats[6]={10,10,10,10,10,10},points=30,focus=0;
 const char*statName[6]={"힘","민첩","체력","매력","지혜","지능"};
 for(;;){
  dos_video_clear(1);menuBorder();
  menuText(r,80,30,"특성치 배분");
  char line[80];
  snprintf(line,sizeof(line),"남은 점수: %d",points);menuText(r,56,68,line);
  for(int i=0;i<6;++i){
   snprintf(line,sizeof(line),"%c %s : %d",i==focus?'>':' ',statName[i],stats[i]);
   menuText(r,82,104+i*37,line);
  }
  menuText(r,40,345,"방향키 조정  Enter 확인");
  int k=getch();
  if(k==27)return false;
  if(k==13&&points==0)break;
  if(k==0||k==224){
   int scan=getch();
   if(scan==72)focus=(focus+5)%6;
   if(scan==80)focus=(focus+1)%6;
   if(scan==77 && points>0){stats[focus]++;points--;}
   if(scan==75 && stats[focus]>10){stats[focus]--;points++;}
  }
 }
 const char*raceName[4]={"인간","엘프","드워프","보빗"};
 const char*sexName[2]={"남성","여성"};
 const char*className[4]={"전사","성직자","마법사","도적"};
 int race=0,sex=0,klass=0;
 for(int stage=0;stage<3;stage++){
  int*choice=stage==0?&race:stage==1?&sex:&klass;
  int count=stage==1?2:4;
  const char**names=stage==0?raceName:stage==1?sexName:className;
  for(;;){
   dos_video_clear(1);menuBorder();
   menuText(r,60,25,stage==0?"종족 선택":stage==1?"성별 선택":"직업 선택");
   for(int i=0;i<count;i++){
    char line[64];snprintf(line,sizeof(line),"%c %d. %s",i==*choice?'>':' ',i+1,names[i]);
    menuText(r,80,88+i*48,line);
   }
   menuText(r,55,346,"방향키  Enter 확인");
   int k=getch();
   if(k==27)return false;
   if(k==13)break;
   if(k>='1'&&k<'1'+count){*choice=k-'1';break;}
   if(k==0||k==224){
    k=getch();
    if(k==72)*choice=(*choice+count-1)%count;
    if(k==80)*choice=(*choice+1)%count;
   }
  }
 }
 dosApplyCreationBonuses(stats,race,klass);

 // Never overwrite the original save slot: export a 820-byte candidate.
 uint8_t record[820]={0};
 for(unsigned i=0;i<length;i++)record[i]=name[i];
 auto put=[&](unsigned off,unsigned value){record[off]=(uint8_t)value;record[off+1]=(uint8_t)(value>>8);};
 put(0x10,race);put(0x12,klass);put(0x14,sex);
 put(0x16,150);
 for(int i=0;i<6;i++)put(0x18+i*2,stats[i]);
 put(0x24,100);put(0x28,200);
 put(0x2A,1);put(0x2E,1);put(0x30,0);
 put(0x32,0x1234);put(0x34,80);put(0x36,74);
 put(0x38,1);put(0xA8,0xFFFF);
 char filename[20];snprintf(filename,sizeof(filename),"PLAYER%d.NEW",slot);
 FILE*f=fopen(filename,"wb");if(!f)return false;
 bool ok=fwrite(record,1,sizeof(record),f)==sizeof(record);
 fclose(f);
 return ok;
}
// Interactive launcher: choose one of the four original DOS save slots.
// Tests bypass this menu entirely, so oracle mode remains deterministic.
static int dosChooseSaveSlot(){
 SDL_Renderer*r=nullptr;
 for(;;){
  dos_video_clear(1);menuBorder();
  menuText(r,170,34,"울티마 I 한글판");
  menuText(r,118,84,"저장된 캐릭터 선택");
  for(int slot=1;slot<=4;++slot){
   char file[16],label[64],name[64];
   snprintf(file,sizeof(file),"PLAYER%d.U1",slot);
   FILE*f=fopen(file,"rb");
   bool valid=false;
   uint8_t record[820]={0};
   if(f){valid=fread(record,1,820,f)==820;fclose(f);}
   if(valid){
    dosSavedNameToUtf8(record,name);
    snprintf(label,sizeof(label),"%d. %s",slot,name);
   }else snprintf(label,sizeof(label),"%d. 빈 슬롯",slot);
   menuText(r,66,130+(slot-1)*48,label);
  }
  menuText(r,50,348,"1~4 선택  Q 종료");
  int c=getch();
  if(c=='q'||c=='Q'||c==27)return 0;
  if(c>='1'&&c<='4')return c-'0';
 }
}
int main(int argc,char**argv){
 FILE*f=fopen("EGATILES.BIN","rb");if(!f){puts("EGATILES.BIN missing");return 1;}
 size_t n=fread(raw,1,6656,f);fclose(f);if(n!=6656)return 2;
 f=fopen("MAP.BIN","rb");if(!f){puts("MAP.BIN missing");return 3;}
 n=fread(mapdata,1,13104,f);fclose(f);if(n!=13104)return 4;
 dosPcmInit();
 FILE*hf=fopen("H12.FNT","rb");
 if(hf){hangulAvailable=fread(hangul,1,sizeof(hangul),hf)==sizeof(hangul);fclose(hf);}
 FILE*font=fopen("ROBO.FNT","rb");
 if(font){fontAvailable=fread(latin,1,4096,font)==4096;fclose(font);}
 const bool launcherInteractive=(argc==1);
 int selectedSlot=1;
 if(launcherInteractive){
  // The temporary dosChooseSaveSlot UI is NOT from original DOS or HF93.
  // Until HF93 FrontendSystem is ported, never present it as the game launcher.
  puts("HF93 frontend not ported yet; temporary slot menu disabled.");
  puts("Run FIELD.EXE PLAY 1 to load PLAYER1.U1 for field testing.");
  puts("PLAY 2/3/4 selects other save slots. No fake frontend.");
  return 0;
 }
 if(argc>=2 && strcmp(argv[1],"PLAY")==0){
  if(argc<3 || argv[2][0]<'1' || argv[2][0]>'4'){
   puts("Usage: FIELD.EXE PLAY 1..4");return 1;
  }
  selectedSlot=argv[2][0]-'0';
 }
 char savePath[16];
 snprintf(savePath,sizeof(savePath),"PLAYER%d.U1",selectedSlot);
 int x=80,y=74,moves=0;
 FILE*save=fopen(savePath,"rb");
 if(!save && launcherInteractive){
  if(dosGenerateCharacter(selectedSlot)){
   // The candidate is not a playable original save until creation is finalized.
   dos_video_clear(1);
   textKorean(nullptr,10,75,"새 캐릭터 후보를 저장했습니다");
   textKorean(nullptr,10,105,"원본 저장 파일은 변경되지 않습니다");
   getch();
  }
  dos_video_end();
  return 0;
 }
 if(save){
  size_t count=fread(saved,1,820,save);fclose(save);
  if(count==820){saveValid=1;transport=saved[0x30]|(saved[0x31]<<8);
   int sx=saved[0x34]|(saved[0x35]<<8),sy=saved[0x36]|(saved[0x37]<<8);
   if(sx>=0&&sx<168&&sy>=0&&sy<156){x=sx;y=sy;}
  }
 }
 OriginalData original;
 LocalSimulation sim(original);
 sim.seed(saveValid?(saved[0x32]|(saved[0x33]<<8)):0x1234);
 OverworldSimulation overworld(original,sim);
 PlayerSave player;
 player.x=x;player.y=y;
 player.hits=saveValid?valueAt(0x16):100;
 player.food=saveValid?valueAt(0x28):100;
 player.experience=saveValid?valueAt(0x26):0;
 player.coin=saveValid?valueAt(0x24):0;
 player.transport=transport;
 if(saveValid)for(int i=0;i<40;i++)for(int j=0;j<8;j++){
  int k=0xB4+i*16+j*2;
  player.worldObjects[i].w[j]=saved[k]|(saved[k+1]<<8);
 }
 livePlayer=&player;
 int engineTurns=0;
 int startx=x,starty=y;
 // TEST mode is read-only; interactive mode exports a separate save candidate.
 bool moveSoundTest=(argc>1 && argv[1][0]=='M');
 bool questTest=(argc>1 && argv[1][0]=='J');
 bool waterTest=(argc>1 && argv[1][0]=='W');
 bool collisionTest=(argc>1 && argv[1][0]=='C');
 bool interactive=(argc==1)||(argc>1 && argv[1][0]=='S');
 bool changed=false;
 auto apply_direction=[&](int dx,int dy){
  int ox=x,oy=y;
  int nx=(x+dx+168)%168,ny=(y+dy+156)%156;
  if(can_move(nx,ny)){
   commandNotice[0]=0;
   if(transport<=2){++landMoveEvents;outdoor_land_sound();} // OUT.EXE sound(4)
   x=nx;y=ny;moves++;changed=true;
   setPlayerPosition(player,x,y);overworld.onPlayerMoved(player,ox,oy);
  }else{
   changed=true;
   ++collisionCount;
   int target=cell(nx,ny);
   const char* msg=target>=19&&target<=47?"괴수 때문에 길이 막혔습니다!":target==3?"산을 통과할 수 없습니다!":target==0?"물 위를 걸을 수 없습니다!":target==2&&transport==5?"비행선은 숲을 통과할 수 없습니다!":(transport==3||transport==4)?"뗏목과 범선은 바다를 좋아합니다!":"육지에서는 이동할 수 없습니다!";
   snprintf(commandNotice,sizeof(commandNotice),"%s",msg);
   if(target>=19&&target<=47)++monsterBlockedEvents;
   else{++terrainSoundEvents;terrain_collision_sound();} // OUT.EXE sound(0)
  }
  overworld.maybeSpawn(player);
  overworld.updateAfterCommand(player);
  sync_world(player);sync_status(player);engineTurns++;
  draw_view(x,y);
 };
 if(!launcherInteractive)dos_video_start();draw_view(x,y);
 if(argc>1 && argv[1][0]=='F'){
  unsigned before=hangulPixels;
  dosFont16Screen(false);
  unsigned added=hangulPixels-before;
  bool pass=dos_video_mode()==0x101&&hangulAvailable&&added>1000;
  FILE*ff=fopen("FONTAUD.OK","wb");
  if(ff){fprintf(ff,"FONT16 %s MODE=%03X GLYPHPIXELS=%u\n",pass?"PASS":"FAIL",dos_video_mode(),added);fclose(ff);}
  goto finish;
 }
 if(questTest){
  bool ok=dosQuestAudit() && dosCp949Audit() && dosCreationAudit();
  FILE* jf=fopen("QUEST.OK","wb");
  if(jf){fprintf(jf,"QUEST %s XP0=%d SLOT2=%d GEM0=%u\n",ok?"PASS":"FAIL",dosShouldAutoHelp(saved)?1:0,int(dosQuestStatus(saved,2)),unsigned(dosWord(saved,0x4C)));fclose(jf);}
  goto finish;
 }
 if(moveSoundTest){
  int sx=-1,sy=-1,dx=0,dy=0;
  for(int yy=1;yy<155&&sx<0;++yy)for(int xx=1;xx<167&&sx<0;++xx){
   if(terrain(xx,yy)!=1)continue;
   if(terrain(xx+1,yy)==1){sx=xx;sy=yy;dx=1;}
  }
  if(sx>=0){transport=0;setPlayerPosition(player,sx,sy);x=sx;y=sy;apply_direction(dx,dy);}
  FILE*mf=fopen("MOVESND.OK","wb");
  if(mf){fprintf(mf,"MOVE SOUND %s MOVES=%d SOUND4=%u SOUND0=%u TURNS=%d PCMREADY=%d PCMPLAY=%u\n",moves==1&&landMoveEvents==1&&terrainSoundEvents==0&&engineTurns==1&&dosPcmReady[1]&&dosPcmEvents[1]==1?"PASS":"FAIL",moves,landMoveEvents,terrainSoundEvents,engineTurns,dosPcmReady[1],dosPcmEvents[1]);fclose(mf);}
  goto finish;
 }
 if(waterTest){
  int sx=-1,sy=-1,dx=0,dy=0;
  for(int yy=1;yy<155&&sx<0;++yy)for(int xx=1;xx<167&&sx<0;++xx){
   if(terrain(xx,yy)!=1)continue;
   if(terrain(xx+1,yy)==0){sx=xx;sy=yy;dx=1;}
   else if(terrain(xx-1,yy)==0){sx=xx;sy=yy;dx=-1;}
   else if(terrain(xx,yy+1)==0){sx=xx;sy=yy;dy=1;}
  }
  int pass=0;
  if(sx>=0){transport=0;setPlayerPosition(player,sx,sy);x=sx;y=sy;apply_direction(dx,dy);
   pass=(x==sx&&y==sy&&moves==0&&engineTurns==1&&terrainSoundEvents==1&&dosPcmEvents[0]==1);
  }
  FILE*wf=fopen("WATER.OK","wb");
  if(wf){fprintf(wf,"WATER %s X=%d Y=%d TURNS=%d SOUND0=%u GLYPHS=%u\n",pass?"PASS":"FAIL",x,y,engineTurns,terrainSoundEvents,hangulGlyphs);fclose(wf);}
  goto finish;
 }
 if(collisionTest){
  int tx=-1,ty=-1,fromX=-1,fromY=-1,dx=0,dy=0;
  // Find adjacent traversable ground and mountain, independently of save location.
  for(int yy=1;yy<155&&tx<0;++yy)for(int xx=1;xx<167&&tx<0;++xx){
   if(terrain(xx,yy)!=1)continue;
   static const int dirs[4][2]={{1,0},{-1,0},{0,1},{0,-1}};
   for(int k=0;k<4;++k)if(terrain(xx+dirs[k][0],yy+dirs[k][1])==3){
    fromX=xx;fromY=yy;dx=dirs[k][0];dy=dirs[k][1];
    tx=xx+dx;ty=yy+dy;break;
   }
  }
  int oldTransport=transport;
  transport=0;
  int blocked=0;
  if(tx>=0){
   setPlayerPosition(player,fromX,fromY);x=fromX;y=fromY;
   int before=engineTurns;
   // Execute the same command handler used by interactive directional movement.
   apply_direction(dx,dy);
   blocked=(x==fromX&&y==fromY&&engineTurns==before+1&&moves==0);
  }
  transport=oldTransport;
  FILE*cf=fopen("COLLISIO.OK","wb");
  if(cf){fprintf(cf,"COLLISION %s FROM=%d,%d TARGET=%d,%d END=%d,%d TURNS=%d COLLISIONS=%u FONT=%d GLYPHS=%u GLYPHPIXELS=%u SOUND0=%u MONSTERBLOCK=%u SOUND4=%u PCMREADY=%d PCMPLAY=%u\n",(blocked&&dos_video_mode()==0x101&&hangulPixels>=500)?"PASS":"FAIL",fromX,fromY,tx,ty,x,y,engineTurns,collisionCount,hangulAvailable,hangulGlyphs,hangulPixels,terrainSoundEvents,monsterBlockedEvents,landMoveEvents,dosPcmReady[0],dosPcmEvents[0]);fclose(cf);}
  goto finish;
 }
 if(argc>1){
  for(int i=0;i<3;i++){
   int nx=(x+1)%168,ny=(y+1)%156;
   if(can_move(nx,ny)){x=nx;y=ny;moves++;changed=true;
  int oldX=(x+167)%168,oldY=(y+155)%156;
  setPlayerPosition(player,x,y);overworld.onPlayerMoved(player,oldX,oldY);
  overworld.maybeSpawn(player);overworld.updateAfterCommand(player);sync_world(player);sync_status(player);engineTurns++;
  draw_view(x,y);}
  }
  goto finish;
 }
 if(interactive && saveValid && dosShouldAutoHelp(saved)){
  usleep(2000000);
  dosHelpScreen();
  draw_view(x,y);
 }
 for(;;){
 int c=getch(); if(c==0||c==224)c=256+getch();
 if(c==27||c=='q'||c=='Q')break;
 if(c=='h'||c=='H'){dosHelpScreen();draw_view(x,y);continue;}
 if(c=='j'||c=='J'){dosQuestScreen(saved);draw_view(x,y);continue;}
 if(c=='f'||c=='F'){dosFont16Screen();draw_view(x,y);continue;}
 int dx=0,dy=0;
 if(c==256+72||c=='w'||c=='W')dy=-1;
 if(c==256+80||c=='s'||c=='S')dy=1;
 if(c==256+75||c=='a'||c=='A')dx=-1;
 if(c==256+77||c=='d'||c=='D')dx=1;
 if(dx||dy)apply_direction(dx,dy);
 }
finish:
 dos_video_end();
 // Export only a separate save candidate; never overwrite the user's original.
 if(interactive && changed && saveValid){
  sync_world(player);sync_status(player);
  saved[0x34]=(unsigned char)(x&255);saved[0x35]=(unsigned char)((x>>8)&255);
  saved[0x36]=(unsigned char)(y&255);saved[0x37]=(unsigned char)((y>>8)&255);
  saved[0xB8]=(unsigned char)(x&255);saved[0xB9]=(unsigned char)((x>>8)&255);
  saved[0xBA]=(unsigned char)(y&255);saved[0xBB]=(unsigned char)((y>>8)&255);
  char exportPath[16];
  snprintf(exportPath,sizeof(exportPath),"PLAYER%d.NEW",selectedSlot);
  FILE*exportFile=fopen(exportPath,"wb");
  if(exportFile){fwrite(saved,1,820,exportFile);fclose(exportFile);}
 }
 f=fopen("FIELD.OK","wb");if(f){fprintf(f,"FIELD VIEW DOS OK START=%d,%d END=%d,%d MOVES=%d TILE=%d TURNS=%d PIXELS=%lu VIDEO=%03X\n",startx,starty,x,y,moves,saveValid?(saved[0xB4]|(saved[0xB5]<<8)):-1,engineTurns,dos_draw_writes(),dos_video_mode());fclose(f);}
 FILE*timing=fopen("PCMTIME.OK","wb");
 if(timing){fprintf(timing,"PCM0 READY=%d CALLS=%u SAMPLES=%u ELAPSED=%lu CLOCK=%lu PCM4 READY=%d CALLS=%u SAMPLES=%u ELAPSED=%lu\n",dosPcmReady[0],dosPcmEvents[0],dosPcmCount[0],dosPcmLastTicks[0],(unsigned long)UCLOCKS_PER_SEC,dosPcmReady[1],dosPcmEvents[1],dosPcmCount[1],dosPcmLastTicks[1]);fclose(timing);}
 return 0;
}