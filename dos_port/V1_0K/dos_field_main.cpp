
#include "OverworldSimulation.h"
#include "SDL.h"
#include "dos_hangul_table.h"
#include <stdio.h>
#include <stdint.h>
#include <conio.h>
#include <unistd.h>
#include <pc.h>
#include "dos_pcm_speaker.h"
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
 SDL_SetRenderDrawColor(r,255,255,255,255);
 for(int i=0;msg[i];++i){
  unsigned c=(unsigned char)msg[i];
  for(int y=0;y<16;++y)for(int x=0;x<8;++x)
   if(latin[c*16+y]&(0x80>>x)) SDL_RenderDrawPoint(r,xx+i*8+x,yy+y);
 }
}
static void textKorean(SDL_Renderer*r,int px,int py,const char*utf8){
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
    for(unsigned y=0;y<16;y++){
     unsigned off=y*2;
     unsigned bits=((unsigned)(hangul[cs*32+off]|hangul[vs*32+off]|hangul[fs*32+off])<<8)|
                   (hangul[cs*32+off+1]|hangul[vs*32+off+1]|hangul[fs*32+off+1]);
     for(unsigned x=0;x<16;x++)if(bits&(0x8000u>>x)){SDL_RenderDrawPoint(r,px+x,py+y);++hangulPixels;}
    }
   }
   px+=16;continue;
  }
  unsigned char c=(unsigned char)u;
  if(fontAvailable){SDL_SetRenderDrawColor(r,255,255,255,255);
   for(int y=0;y<16;y++)for(int x=0;x<8;x++)
    if(latin[c*16+y]&(0x80>>x)) SDL_RenderDrawPoint(r,px+x,py+y);
  }
  px+=8;
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
 dos_video_clear(1);
 SDL_Renderer*r=nullptr;SDL_SetRenderDrawColor(r,255,255,255,255);
 SDL_Rect frame{13,13,294,174};SDL_RenderDrawRect(r,&frame);
 for(int dy=-4;dy<=4;dy++)for(int dx=-7;dx<=7;dx++){
 int tile=cell(cx+dx,cy+dy);
 tile_image(tile,pixeldata);
 SDL_Texture tex{16,16,pixeldata};
 SDL_Rect dst{40+(dx+7)*16,20+(dy+4)*16,16,16};
 SDL_RenderCopy(r,&tex,nullptr,&dst);
 }
 if(saveValid && fontAvailable){
  char line[48];
  sprintf(line,"HP:%u FOOD:%u XP:%u GOLD:%u",
   valueAt(0x16),valueAt(0x28),valueAt(0x26),valueAt(0x24));
  textAt(r,22,169,line);
 }
 if(commandNotice[0]){
  // Dedicated opaque notification strip, avoiding text over terrain sprites.
  SDL_SetRenderDrawColor(r,0,0,0,255);
  SDL_Rect strip{21,148,278,19};SDL_RenderFillRect(r,&strip);
  textKorean(r,22,149,commandNotice);
 }
 // Draw the saved player WorldObject[0] physical tile over the terrain.
 int playerTile=saveValid?(saved[0xB4]|(saved[0xB5]<<8)):-1;
 if(playerTile>=0&&playerTile<52){
  tile_image(playerTile,avatar);
  SDL_Texture playerTex{16,16,avatar};
  SDL_Rect playerDst{40+7*16,20+4*16,16,16};
  SDL_RenderCopy(r,&playerTex,nullptr,&playerDst);
 } else {
  SDL_SetRenderDrawColor(r,255,255,255,255);
  SDL_RenderDrawLine(r,160,85,160,99);
  SDL_RenderDrawLine(r,153,92,167,92);
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
 int x=80,y=74,moves=0;
 FILE*save=fopen("PLAYER1.U1","rb");
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
 dos_video_start();draw_view(x,y);
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
  if(cf){fprintf(cf,"COLLISION %s FROM=%d,%d TARGET=%d,%d END=%d,%d TURNS=%d COLLISIONS=%u FONT=%d GLYPHS=%u GLYPHPIXELS=%u SOUND0=%u MONSTERBLOCK=%u SOUND4=%u PCMREADY=%d PCMPLAY=%u\n",blocked?"PASS":"FAIL",fromX,fromY,tx,ty,x,y,engineTurns,collisionCount,hangulAvailable,hangulGlyphs,hangulPixels,terrainSoundEvents,monsterBlockedEvents,landMoveEvents,dosPcmReady[0],dosPcmEvents[0]);fclose(cf);}
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
 for(;;){
 int c=getch(); if(c==0||c==224)c=256+getch();
 if(c==27||c=='q'||c=='Q')break;
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
  FILE*exportFile=fopen("PLAYER1.NEW","wb");
  if(exportFile){fwrite(saved,1,820,exportFile);fclose(exportFile);}
 }
 f=fopen("FIELD.OK","wb");if(f){fprintf(f,"FIELD VIEW DOS OK START=%d,%d END=%d,%d MOVES=%d TILE=%d TURNS=%d PIXELS=%lu\n",startx,starty,x,y,moves,saveValid?(saved[0xB4]|(saved[0xB5]<<8)):-1,engineTurns,dos_draw_writes());fclose(f);}
 FILE*timing=fopen("PCMTIME.OK","wb");
 if(timing){fprintf(timing,"PCM0 READY=%d CALLS=%u SAMPLES=%u ELAPSED=%lu CLOCK=%lu PCM4 READY=%d CALLS=%u SAMPLES=%u ELAPSED=%lu\n",dosPcmReady[0],dosPcmEvents[0],dosPcmCount[0],dosPcmLastTicks[0],(unsigned long)UCLOCKS_PER_SEC,dosPcmReady[1],dosPcmEvents[1],dosPcmCount[1],dosPcmLastTicks[1]);fclose(timing);}
 return 0;
}
