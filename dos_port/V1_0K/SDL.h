
#pragma once
#include <stdint.h>
struct SDL_Renderer;
struct SDL_Texture { int w,h; const uint8_t* pixels; };
struct SDL_Surface;
struct SDL_Color { uint8_t r,g,b,a; };
struct SDL_Rect {int x,y,w,h;};
extern "C" {
int SDL_RenderFillRect(SDL_Renderer*,const SDL_Rect*);
int SDL_SetRenderDrawColor(SDL_Renderer*,uint8_t,uint8_t,uint8_t,uint8_t);
int SDL_RenderDrawLine(SDL_Renderer*,int,int,int,int);
int SDL_RenderDrawPoint(SDL_Renderer*,int,int);
int SDL_RenderDrawRect(SDL_Renderer*,const SDL_Rect*);
int SDL_RenderCopy(SDL_Renderer*,SDL_Texture*,const SDL_Rect*,const SDL_Rect*);
}
extern "C" uint32_t SDL_GetTicks(void);
