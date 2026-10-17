#include "SDL.h"
#include <stdlib.h>
#include <string.h>
// 32-bit ARGB render target used by the unmodified HF93 640x400 pipeline.
// Offscreen operations must be wired into the SDL drawing backend before runtime.
struct Hf93Target { SDL_Texture texture; uint32_t* argb; int access; };
static SDL_Texture* currentTarget=0;
static bool hf93OffscreenMode=false;
extern "C" int hf93PresentationMode(void){return hf93OffscreenMode?1:0;}
extern "C" SDL_Texture* SDL_CreateTexture(SDL_Renderer*,Uint32,int access,int w,int h){
 if(w<=0||h<=0||w>2048||h>2048)return nullptr;
 Hf93Target* t=(Hf93Target*)calloc(1,sizeof(Hf93Target));
 if(!t)return nullptr;
 t->argb=(uint32_t*)calloc((size_t)w*h,sizeof(uint32_t));
 if(!t->argb){free(t);return nullptr;}
 t->texture.w=w;t->texture.h=h;t->texture.pixels=(const uint8_t*)t->argb;
 t->access=access;return &t->texture;
}
extern "C" void SDL_DestroyTexture(SDL_Texture* tex){
 if(!tex)return;
 if(currentTarget==tex)currentTarget=0;
 Hf93Target* t=(Hf93Target*)tex;free(t->argb);free(t);
}
extern "C" int SDL_SetRenderTarget(SDL_Renderer*,SDL_Texture* tex){
 if(tex&&((Hf93Target*)tex)->access!=SDL_TEXTUREACCESS_TARGET)return -1;
 if(tex)hf93OffscreenMode=true;
 currentTarget=tex;return 0;
}
extern "C" int SDL_SetTextureBlendMode(SDL_Texture*,int){return 0;}
extern "C" int SDL_UpdateTexture(SDL_Texture* tex,const SDL_Rect* area,const void* src,int pitch){
 if(!tex||!src||pitch<=0)return -1;
 Hf93Target* t=(Hf93Target*)tex;
 SDL_Rect a=area?*area:SDL_Rect{0,0,tex->w,tex->h};
 if(a.x<0||a.y<0||a.w<=0||a.h<=0||a.x+a.w>tex->w||a.y+a.h>tex->h)return -1;
 for(int y=0;y<a.h;y++)memcpy(t->argb+(a.y+y)*tex->w+a.x,(const char*)src+y*pitch,a.w*4);
 return 0;
}
extern "C" int SDL_RenderReadPixels(SDL_Renderer*,const SDL_Rect* a,Uint32 fmt,void* dst,int pitch){
 if(!currentTarget||!dst||fmt!=SDL_PIXELFORMAT_ARGB8888||pitch<=0)return -1;
 Hf93Target* t=(Hf93Target*)currentTarget;
 SDL_Rect r=a?*a:SDL_Rect{0,0,t->texture.w,t->texture.h};
 if(r.x<0||r.y<0||r.x+r.w>t->texture.w||r.y+r.h>t->texture.h)return -1;
 for(int y=0;y<r.h;y++)memcpy((char*)dst+y*pitch,t->argb+(r.y+y)*t->texture.w+r.x,r.w*4);
 return 0;
}
extern "C" void SDL_StartTextInput(void){}
// Expose the target to the DOS renderer so it can be used instead of drawing
// directly to VESA while HF93 is composing a game frame.
extern "C" uint32_t* hf93TargetPixels(void){return currentTarget?((Hf93Target*)currentTarget)->argb:nullptr;}
extern "C" int hf93TargetWidth(void){return currentTarget?currentTarget->w:0;}
extern "C" int hf93TargetHeight(void){return currentTarget?currentTarget->h:0;}