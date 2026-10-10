
#include "SDL.h"
#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <time.h>
#include <stdint.h>
#include <stdlib.h>
struct SDL_Renderer { int color; };
static SDL_Renderer renderer={15};
static unsigned long writes=0;
static void pixel(int x,int y){
 if((unsigned)x>=320u||(unsigned)y>=200u) return;
 _farpokeb(_dos_ds,0xA0000UL+(unsigned long)y*320u+(unsigned)x,renderer.color);
 ++writes;
}
extern "C" int SDL_SetRenderDrawColor(SDL_Renderer*,uint8_t r,uint8_t g,uint8_t b,uint8_t){
 renderer.color=(r>128?4:0)|(g>128?2:0)|(b>128?1:0);
 if(r>220&&g>220&&b>220) renderer.color=15;
 return 0;
}
extern "C" int SDL_RenderDrawPoint(SDL_Renderer*,int x,int y){pixel(x,y);return 0;}
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
extern "C" int SDL_RenderCopy(SDL_Renderer*,SDL_Texture* tex,const SDL_Rect* src,const SDL_Rect* dst){
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
   if((unsigned)px>=320u||(unsigned)py>=200u)continue;
   _farpokeb(_dos_ds,0xA0000UL+(unsigned long)py*320u+(unsigned)px,tex->pixels[sy*tex->w+sx]);
   ++writes;
  }
 }
 return 0;
}
extern "C" uint32_t SDL_GetTicks(void){return (uint32_t)((double)clock()*1000.0/CLOCKS_PER_SEC);}
extern "C" int dos_video_start(void){
 __dpmi_regs regs={}; regs.x.ax=0x13; __dpmi_int(0x10,&regs);return 0;
}
extern "C" void dos_video_end(void){
 __dpmi_regs regs={};regs.x.ax=3;__dpmi_int(0x10,&regs);
}
extern "C" void dos_video_clear(int c){
 renderer.color=c;
 for(unsigned long i=0;i<64000;++i)_farpokeb(_dos_ds,0xA0000UL+i,c);
}
extern "C" unsigned long dos_draw_writes(void){return writes;}
