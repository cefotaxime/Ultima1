#include "SDL.h"
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
struct SDL_Renderer { int color; };
static SDL_Renderer renderer={15};
static unsigned long writes=0;
static unsigned char clearBank[65536];
static int vesa=0,bank=-1;
static void setBank(int b){
 if(b==bank)return;
 __dpmi_regs reg={};
 reg.x.ax=0x4F05;reg.x.bx=0;reg.x.dx=b;
 __dpmi_int(0x10,&reg);
 bank=b;
}
static void physicalPixel(int x,int y,int color){
 if((unsigned)x>=640u||(unsigned)y>=480u)return;
 unsigned long off=(unsigned long)y*640u+(unsigned)x;
 setBank((int)(off>>16));
 _farpokeb(_dos_ds,0xA0000UL+(off&65535u),color);
}
// Physical 640x400 presentation within VESA 640x480 (40px letterbox).
// One-pixel cyan/white rectangular borders, no scaling of glyph pixels.
static void hf93FrameRect(int x,int y,int w,int h,int color){
 if(!vesa)return;
 for(int yy=y*2;yy<(y+h)*2;yy++)for(int xx=x*2;xx<(x+w)*2;xx++)
  physicalPixel(xx,yy+40,color);
}
extern "C" void dos_vesa_frame(void){
 if(!vesa)return;
 // Exact HF93 main.cpp drawFrames 320x200 EGA source coordinates.
 static const int inset[7]={7,5,4,3,2,2,1};
 for(int y=0;y<7;y++)hf93FrameRect(inset[y],y,320-2*inset[y],1,1);
 hf93FrameRect(0,7,7,145,1);
 hf93FrameRect(313,7,7,145,1);
 hf93FrameRect(0,152,320,7,1);
 hf93FrameRect(242,160,4,40,1);
 hf93FrameRect(8,7,304,1,15);
 hf93FrameRect(7,8,1,143,15);
 hf93FrameRect(312,8,1,143,15);
 hf93FrameRect(8,151,304,1,15);
 hf93FrameRect(0,159,242,1,15);
 hf93FrameRect(241,160,1,40,15);
 hf93FrameRect(246,159,74,1,15);
 hf93FrameRect(246,160,1,40,15);
}
extern "C" void hf93_physical_pixel(int x,int y,int color){if(vesa)physicalPixel(x,y,color);}
// Commit a complete indexed frame using bank-sized writes, not per-pixel BIOS bank switches.
extern "C" int hf93_commit_indexed(const unsigned char* framebuffer){
 if(!vesa||!framebuffer)return -1;
 static unsigned char previous[640u*480u];
 static bool valid=false;
 for(unsigned y=0;y<480;y++){
  const unsigned start=y*640u;
  if(valid && memcmp(previous+start,framebuffer+start,640u)==0)continue;
  memcpy(previous+start,framebuffer+start,640u);
  unsigned offset=start,remaining=640;
  while(remaining){
   unsigned bank=offset>>16;unsigned inBank=offset&65535u;
   unsigned count=65536u-inBank;if(count>remaining)count=remaining;
   setBank(bank);dosmemput(framebuffer+offset,count,0xA0000UL+inBank);
   offset+=count;remaining-=count;
  }
 }
 valid=true;
 return 0;
}
extern "C" int hf93PresentationMode(void);

extern "C" void dos_ui_pixel(int x,int y,int color){
 if(vesa && (unsigned)x<640u && (unsigned)y<400u)
  physicalPixel(x,y+40,color);
}
extern "C" void dos_ui_rect(int x,int y,int w,int h,int color){
 if(!vesa || w<=0 || h<=0)return;
 for(int xx=x;xx<x+w;xx++){dos_ui_pixel(xx,y,color);dos_ui_pixel(xx,y+h-1,color);}
 for(int yy=y;yy<y+h;yy++){dos_ui_pixel(x,yy,color);dos_ui_pixel(x+w-1,yy,color);}
}
extern "C" int dos_native_hangul(void){return vesa;}
extern "C" int dos_video_mode(void){return vesa?0x101:0x13;}
extern "C" void dos_native_glyph_pixel(int x,int y){
 if(vesa)physicalPixel(x,40+y,15);
}
extern "C" void dos_native_pixel(int x,int y,int color){
 if(vesa)physicalPixel(2*x,40+2*y,color);
}

extern "C" uint32_t* hf93TargetPixels(void);
extern "C" int hf93TargetWidth(void);
extern "C" int hf93TargetHeight(void);
static void pixel(int x,int y){
 uint32_t* target=hf93TargetPixels();
 if(target){
  if((unsigned)x<(unsigned)hf93TargetWidth() && (unsigned)y<(unsigned)hf93TargetHeight()){
   static const uint32_t ega[16]={0xFF000000u,0xFF0000AAu,0xFF00AA00u,0xFF00AAAAu,0xFFAA0000u,0xFFAA00AAu,0xFFAA5500u,0xFFAAAAAAu,0xFF555555u,0xFF5555FFu,0xFF55FF55u,0xFF55FFFFu,0xFFFF5555u,0xFFFF55FFu,0xFFFFFF55u,0xFFFFFFFFu};
   target[y*hf93TargetWidth()+x]=ega[renderer.color&15];
  }
  return;
 }
 if((unsigned)x>=320u||(unsigned)y>=200u) return;
 if(vesa){
  int xx=x*2,yy=40+y*2;
  physicalPixel(xx,yy,renderer.color);
  physicalPixel(xx+1,yy,renderer.color);
  physicalPixel(xx,yy+1,renderer.color);
  physicalPixel(xx+1,yy+1,renderer.color);
 }else _farpokeb(_dos_ds,0xA0000UL+(unsigned long)y*320u+(unsigned)x,renderer.color);
 ++writes;
}
extern "C" int SDL_SetRenderDrawColor(SDL_Renderer*,uint8_t r,uint8_t g,uint8_t b,uint8_t){
 // Preserve all sixteen EGA canonical RGB values through the DOS indexed frame.
 static const uint8_t ega[16][3]={
  {0,0,0},{0,0,170},{0,170,0},{0,170,170},
  {170,0,0},{170,0,170},{170,85,0},{170,170,170},
  {85,85,85},{85,85,255},{85,255,85},{85,255,255},
  {255,85,85},{255,85,255},{255,255,85},{255,255,255}
 };
 // Exact canonical match first, closest EGA entry otherwise.
 int best=0; unsigned bestDist=~0u;
 for(int i=0;i<16;++i){
  int dr=int(r)-int(ega[i][0]),dg=int(g)-int(ega[i][1]),db=int(b)-int(ega[i][2]);
  unsigned dist=unsigned(dr*dr+dg*dg+db*db);
  if(dist<bestDist){best=i;bestDist=dist;}
 }
 // The SPACE.EXE stellar raster intentionally requests saturated red/yellow
 // which must use bright EGA indexes, not their nearest dark counterparts.
 if(r==255 && g==0 && b==0)best=12;
 if(r==255 && g==255 && b==0)best=14;
 renderer.color=best;
 return 0;
}extern "C" int SDL_RenderDrawPoint(SDL_Renderer*,int x,int y){pixel(x,y);return 0;}
extern "C" void dos_video_clear(int);
extern "C" int SDL_RenderClear(SDL_Renderer*){
 uint32_t* target=hf93TargetPixels();
 if(target){
  const uint32_t ega[16]={0xFF000000u,0xFF0000AAu,0xFF00AA00u,0xFF00AAAAu,0xFFAA0000u,0xFFAA00AAu,0xFFAA5500u,0xFFAAAAAAu,0xFF555555u,0xFF5555FFu,0xFF55FF55u,0xFF55FFFFu,0xFFFF5555u,0xFFFF55FFu,0xFFFFFF55u,0xFFFFFFFFu};
  const int n=hf93TargetWidth()*hf93TargetHeight();
  for(int i=0;i<n;i++)target[i]=ega[renderer.color&15];
  return 0;
 }
 if(!hf93PresentationMode())dos_video_clear(renderer.color);return 0;
}
extern "C" int SDL_RenderFillRect(SDL_Renderer*,const SDL_Rect* p){
 if(!p)return -1;
 for(int y=p->y;y<p->y+p->h;++y)for(int x=p->x;x<p->x+p->w;++x)pixel(x,y);
 return 0;
}
extern "C" int SDL_RenderDrawLine(SDL_Renderer*,int x,int y,int xx,int yy){
 int dx=abs(xx-x),sx=x<xx?1:-1,dy=-abs(yy-y),sy=y<yy?1:-1,err=dx+dy;
 for(;;){pixel(x,y);if(x==xx&&y==yy)break;int e=err*2;if(e>=dy){err+=dy;x+=sx;}if(e<=dx){err+=dx;y+=sy;}}
 return 0;
}
extern "C" int SDL_RenderDrawRect(SDL_Renderer*r,const SDL_Rect*p){
 if(!p)return -1;SDL_RenderDrawLine(r,p->x,p->y,p->x+p->w-1,p->y);
 SDL_RenderDrawLine(r,p->x,p->y,p->x,p->y+p->h-1);
 SDL_RenderDrawLine(r,p->x+p->w-1,p->y,p->x+p->w-1,p->y+p->h-1);
 SDL_RenderDrawLine(r,p->x,p->y+p->h-1,p->x+p->w-1,p->y+p->h-1);return 0;
}
extern "C" int hf93PresentARGB(const uint32_t*,int,int);
extern "C" int SDL_RenderCopy(SDL_Renderer*,SDL_Texture* tex,const SDL_Rect* src,const SDL_Rect* dst){
 // HF93 presentFrame copies its 640x400 target to the physical display.
 // The host-side destination is 4:3; DOS mode already supplies this aspect.
 if(!hf93TargetPixels() && tex && tex->w==640 && tex->h==400 && dst && dst->w>=640 && dst->h>=400)
  return hf93PresentARGB((const uint32_t*)tex->pixels,tex->w,tex->h);
 if(!tex||!tex->pixels||tex->w<=0||tex->h<=0)return -1;
 SDL_Rect s=src?*src:SDL_Rect{0,0,tex->w,tex->h};
 SDL_Rect d=dst?*dst:SDL_Rect{0,0,s.w,s.h};
 if(s.w<=0||s.h<=0||d.w<=0||d.h<=0)return -1;
 for(int y=0;y<d.h;y++){
  int sy=s.y+(int)((long long)y*s.h/d.h);
  if(sy<0||sy>=tex->h)continue;
  for(int x=0;x<d.w;x++){
   int sx=s.x+(int)((long long)x*s.w/d.w);
   if(sx<0||sx>=tex->w)continue;
   int px=d.x+x,py=d.y+y;
   uint32_t* target=hf93TargetPixels();
   if(target){
    if((unsigned)px<(unsigned)hf93TargetWidth() && (unsigned)py<(unsigned)hf93TargetHeight()){
     // HF93 render targets and streamed textures are 32-bit ARGB.
     const uint32_t* source=(const uint32_t*)tex->pixels;
     target[py*hf93TargetWidth()+px]=source[sy*tex->w+sx];
    }
    continue;
   }
   if((unsigned)px>=320u||(unsigned)py>=200u)continue;
   int c=tex->pixels[sy*tex->w+sx];
   if(vesa){physicalPixel(px*2,40+py*2,c);physicalPixel(px*2+1,40+py*2,c);
    physicalPixel(px*2,41+py*2,c);physicalPixel(px*2+1,41+py*2,c);}
   else _farpokeb(_dos_ds,0xA0000UL+(unsigned long)py*320u+(unsigned)px,c);
   ++writes;
  }
 }
 return 0;
}
extern "C" uint32_t SDL_GetTicks(void){return (uint32_t)((double)clock()*1000.0/CLOCKS_PER_SEC);}
extern "C" int dos_video_start(void){
 __dpmi_regs regs={};
 regs.x.ax=0x4F02;regs.x.bx=0x101;__dpmi_int(0x10,&regs);
 vesa=(regs.x.ax==0x004F);
 bank=-1;
 if(!vesa){regs.x.ax=0x13;__dpmi_int(0x10,&regs);}
 return 0;
}
extern "C" void dos_video_end(void){
 __dpmi_regs regs={};regs.x.ax=3;__dpmi_int(0x10,&regs);
}
extern "C" void dos_video_clear(int c){
 renderer.color=c;
 if(vesa){
  memset(clearBank,c,sizeof(clearBank));
  for(int b=0;b<5;b++){
   setBank(b);
   dosmemput(clearBank,sizeof(clearBank),0xA0000UL);
  }
 }else{
  memset(clearBank,c,sizeof(clearBank));
  dosmemput(clearBank,64000u,0xA0000UL);
 }
}
extern "C" unsigned long dos_draw_writes(void){return writes;}