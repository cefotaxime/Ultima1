#include "SDL.h"
#include <stdlib.h>
#include <string.h>
extern "C" SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32,int w,int h,int depth,Uint32){
 if(w<=0||h<=0||depth!=32)return nullptr;
 SDL_Surface* s=(SDL_Surface*)calloc(1,sizeof(SDL_Surface));if(!s)return nullptr;
 s->w=w;s->h=h;s->pitch=w*4;
 s->pixels=calloc((size_t)h,(size_t)s->pitch);
 s->format=(SDL_PixelFormat*)calloc(1,sizeof(SDL_PixelFormat));
 if(!s->pixels||!s->format){free(s->pixels);free(s->format);free(s);return nullptr;}
 return s;
}
extern "C" Uint32 SDL_MapRGBA(const SDL_PixelFormat*,Uint8 r,Uint8 g,Uint8 b,Uint8 a){return ((Uint32)a<<24)|((Uint32)r<<16)|((Uint32)g<<8)|b;}
extern "C" void SDL_FreeSurface(SDL_Surface* s){if(s){free(s->pixels);free(s->format);free(s);}}
extern "C" SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer* r,SDL_Surface* s){
 if(!s)return nullptr;
 SDL_Texture* t=SDL_CreateTexture(r,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,s->w,s->h);
 if(t && SDL_UpdateTexture(t,nullptr,s->pixels,s->pitch)!=0){SDL_DestroyTexture(t);return nullptr;}
 return t;
}