#include "SDL.h"
#include <stdio.h>
// Real DOS video-mode lifecycle; no substitute title/menu/game content.
extern "C" int dos_video_start(void);
extern "C" void dos_video_end(void);
static SDL_Window window={640,400};
extern "C" int SDL_Init(Uint32){int x=dos_video_start();FILE* f=fopen("WINDOW.LOG","w");if(f){fprintf(f,"video_start=%d\n",x);fclose(f);}return x;}
extern "C" void SDL_Quit(void){dos_video_end();}
extern "C" int SDL_SetHint(const char*,const char*){return 1;}
extern "C" SDL_Window* SDL_CreateWindow(const char*,int,int,int w,int h,Uint32){window.width=w;window.height=h;return &window;}
extern "C" SDL_Renderer* SDL_CreateRenderer(SDL_Window*,int,Uint32){return reinterpret_cast<SDL_Renderer*>(1);}
extern "C" void SDL_ShowWindow(SDL_Window*){}
extern "C" void SDL_RaiseWindow(SDL_Window*){}
extern "C" void SDL_RenderPresent(SDL_Renderer*){} // VESA writes are synchronous.
extern "C" void SDL_PumpEvents(void){}
extern "C" int SDL_GetRendererOutputSize(SDL_Renderer*,int* w,int* h){if(w)*w=640;if(h)*h=480;return 0;}
extern "C" int SDL_SetWindowFullscreen(SDL_Window*,Uint32){return 0;}
extern "C" void SDL_SetWindowSize(SDL_Window*,int,int){} // Physical VESA mode fixed.
extern "C" void SDL_DestroyRenderer(SDL_Renderer*){}
extern "C" void SDL_DestroyWindow(SDL_Window*){}