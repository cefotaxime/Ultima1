#include "SDL.h"
#include <stdint.h>
extern "C" uint32_t* hf93TargetPixels(void);
extern "C" int hf93TargetWidth(void);
extern "C" int hf93TargetHeight(void);
extern "C" int SDL_RenderCopyEx(SDL_Renderer* r,SDL_Texture* tex,const SDL_Rect* src,const SDL_Rect* dst,double angle,const void* center,int flip){
 if(!tex||!tex->pixels)return -1;
 if(angle!=0.0||center)return -1; // do not silently approximate unsupported rotation
 if(flip==0)return SDL_RenderCopy(r,tex,src,dst);
 if(flip!=SDL_FLIP_HORIZONTAL)return -1;
 const SDL_Rect s=src?*src:SDL_Rect{0,0,tex->w,tex->h};
 const SDL_Rect d=dst?*dst:SDL_Rect{0,0,s.w,s.h};
 if(s.w<=0||s.h<=0||d.w<=0||d.h<=0)return -1;
 uint32_t* out=hf93TargetPixels();
 if(!out)return -1;
 const int ow=hf93TargetWidth(),oh=hf93TargetHeight();
 const uint32_t* in=(const uint32_t*)tex->pixels;
 for(int y=0;y<d.h;y++)for(int x=0;x<d.w;x++){
  int sy=s.y+(int)((long long)y*s.h/d.h);
  int sx=s.x+s.w-1-(int)((long long)x*s.w/d.w);
  int xx=d.x+x,yy=d.y+y;
  if(sx>=0&&sy>=0&&sx<tex->w&&sy<tex->h&&xx>=0&&yy>=0&&xx<ow&&yy<oh)
   out[yy*ow+xx]=in[sy*tex->w+sx];
 }
 return 0;
}