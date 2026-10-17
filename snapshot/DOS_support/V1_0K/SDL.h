#pragma once
#include <stdint.h>
struct SDL_Renderer;
struct SDL_Texture { int w,h; const uint8_t* pixels; };
struct SDL_PixelFormat { uint32_t unused; };
struct SDL_Surface { int w,h,pitch; void* pixels; SDL_PixelFormat* format; };
struct SDL_Color { uint8_t r,g,b,a; };
struct SDL_Rect {int x,y,w,h;};
extern "C" {
int SDL_RenderFillRect(SDL_Renderer*,const SDL_Rect*);
int SDL_RenderClear(SDL_Renderer*);
int SDL_SetRenderDrawColor(SDL_Renderer*,uint8_t,uint8_t,uint8_t,uint8_t);
int SDL_RenderDrawLine(SDL_Renderer*,int,int,int,int);
int SDL_RenderDrawPoint(SDL_Renderer*,int,int);
int SDL_RenderDrawRect(SDL_Renderer*,const SDL_Rect*);
int SDL_RenderCopy(SDL_Renderer*,SDL_Texture*,const SDL_Rect*,const SDL_Rect*);
}
extern "C" uint32_t SDL_GetTicks(void);

// SDL2-compatible declarations for HF93 original audio interface.
// These are TYPES ONLY; no pretend SDL implementation is supplied.
#ifndef SDLCALL
#define SDLCALL
#endif
typedef uint8_t Uint8;
typedef uint16_t Uint16;
typedef uint32_t Uint32;
typedef Uint32 SDL_AudioDeviceID;
typedef void (SDLCALL *SDL_AudioCallback)(void*,Uint8*,int);
struct SDL_AudioSpec { int freq; Uint16 format; Uint8 channels; Uint8 silence; Uint16 samples; Uint16 padding; Uint32 size; SDL_AudioCallback callback; void* userdata; };
constexpr int SDLK_UNKNOWN=0;
// HF93 original frontend input compatibility. SDL2 keycodes.
typedef int32_t SDL_Keycode;
typedef uint16_t SDL_Keymod;
enum { KMOD_SHIFT=0x0003, KMOD_CAPS=0x2000 };
constexpr SDL_Keycode SDLK_a=97;
constexpr SDL_Keycode SDLK_b=98;
constexpr SDL_Keycode SDLK_c=99;
constexpr SDL_Keycode SDLK_d=100;
constexpr SDL_Keycode SDLK_e=101;
constexpr SDL_Keycode SDLK_f=102;
constexpr SDL_Keycode SDLK_g=103;
constexpr SDL_Keycode SDLK_h=104;
constexpr SDL_Keycode SDLK_i=105;
constexpr SDL_Keycode SDLK_j=106;
constexpr SDL_Keycode SDLK_k=107;
constexpr SDL_Keycode SDLK_l=108;
constexpr SDL_Keycode SDLK_m=109;
constexpr SDL_Keycode SDLK_n=110;
constexpr SDL_Keycode SDLK_o=111;
constexpr SDL_Keycode SDLK_p=112;
constexpr SDL_Keycode SDLK_q=113;
constexpr SDL_Keycode SDLK_r=114;
constexpr SDL_Keycode SDLK_s=115;
constexpr SDL_Keycode SDLK_t=116;
constexpr SDL_Keycode SDLK_u=117;
constexpr SDL_Keycode SDLK_v=118;
constexpr SDL_Keycode SDLK_w=119;
constexpr SDL_Keycode SDLK_x=120;
constexpr SDL_Keycode SDLK_y=121;
constexpr SDL_Keycode SDLK_z=122;
enum { SDLK_0=48, SDLK_9=57, SDLK_BACKSPACE=8, SDLK_RETURN=13, SDLK_ESCAPE=27, SDLK_SPACE=32, SDLK_MINUS=45, SDLK_PERIOD=46, SDLK_QUOTE=39, SDLK_KP_ENTER=1073741912, SDLK_RIGHT=1073741903, SDLK_LEFT=1073741904, SDLK_DOWN=1073741905, SDLK_UP=1073741906 };

// Real DOS implementations live in dos_hf93_platform.cpp, not FIELD.EXE.
extern "C" {
const char* SDL_GetError(void);
char* SDL_GetBasePath(void);
void SDL_free(void*);
void SDL_Delay(uint32_t ms);
}

// SDL keyboard event ABI subset used by HF93; backed by DOS conio.
typedef int SDL_Scancode;
struct SDL_Keysym { SDL_Scancode scancode; SDL_Keycode sym; SDL_Keymod mod; Uint32 unused; };
struct SDL_KeyboardEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; Uint8 state; Uint8 repeat; Uint8 padding2; Uint8 padding3; SDL_Keysym keysym; };
struct SDL_TextInputEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; char text[32]; };
struct SDL_TextEditingEvent { Uint32 type; Uint32 timestamp; Uint32 windowID; char text[32]; int start; int length; };
union SDL_Event { Uint32 type; SDL_KeyboardEvent key; SDL_TextInputEvent text; SDL_TextEditingEvent edit; Uint8 padding[56]; };
enum { SDL_QUIT=0x100, SDL_KEYDOWN=0x300, SDL_KEYUP=0x301, SDL_TEXTEDITING=0x302, SDL_TEXTINPUT=0x303, SDL_PEEKEVENT=1, SDL_SCANCODE_A=4, SDL_SCANCODE_Z=29, KMOD_ALT=0x0300 };
constexpr SDL_Keycode SDLK_1=49, SDLK_8=56;
extern "C" int SDL_PollEvent(SDL_Event*);
extern "C" int SDL_PushEvent(SDL_Event*);
extern "C" int SDL_PeepEvents(SDL_Event*,int,int,Uint32,Uint32);
extern "C" SDL_Keymod SDL_GetModState(void);

// HF93 original SDL video lifecycle; DOS video backend implementation.
struct SDL_Window { int width,height; };
enum { SDL_INIT_VIDEO=0x20, SDL_WINDOW_RESIZABLE=0x20, SDL_WINDOW_FULLSCREEN_DESKTOP=0x1001,
 SDL_WINDOWPOS_CENTERED=0, SDL_RENDERER_ACCELERATED=2, SDL_RENDERER_PRESENTVSYNC=4,
 SDL_RENDERER_TARGETTEXTURE=8, SDL_RENDERER_SOFTWARE=1 };
#define SDL_HINT_RENDER_SCALE_QUALITY "SDL_RENDER_SCALE_QUALITY"
extern "C" {
int SDL_Init(Uint32);
void SDL_Quit(void);
int SDL_SetHint(const char*,const char*);
SDL_Window* SDL_CreateWindow(const char*,int,int,int,int,Uint32);
SDL_Renderer* SDL_CreateRenderer(SDL_Window*,int,Uint32);
void SDL_ShowWindow(SDL_Window*);
void SDL_RaiseWindow(SDL_Window*);
void SDL_RenderPresent(SDL_Renderer*);
void SDL_PumpEvents(void);
int SDL_GetRendererOutputSize(SDL_Renderer*,int*,int*);
int SDL_SetWindowFullscreen(SDL_Window*,Uint32);
void SDL_SetWindowSize(SDL_Window*,int,int);
void SDL_DestroyRenderer(SDL_Renderer*);
void SDL_DestroyWindow(SDL_Window*);
}

// Original HF93 offscreen 640x400 render-target interface.
enum { SDL_PIXELFORMAT_ARGB8888=0x16362004, SDL_TEXTUREACCESS_TARGET=2, SDL_TEXTUREACCESS_STREAMING=1, SDL_BLENDMODE_NONE=0, SDL_FLIP_HORIZONTAL=1 };
extern "C" {
SDL_Texture* SDL_CreateTexture(SDL_Renderer*,Uint32,int,int,int);
void SDL_DestroyTexture(SDL_Texture*);
int SDL_SetRenderTarget(SDL_Renderer*,SDL_Texture*);
int SDL_SetTextureBlendMode(SDL_Texture*,int);
int SDL_UpdateTexture(SDL_Texture*,const SDL_Rect*,const void*,int);
int SDL_RenderReadPixels(SDL_Renderer*,const SDL_Rect*,Uint32,void*,int);
int SDL_RenderCopyEx(SDL_Renderer*,SDL_Texture*,const SDL_Rect*,const SDL_Rect*,double,const void*,int);
void SDL_StartTextInput(void);
}

enum { SDL_PIXELFORMAT_RGBA32=SDL_PIXELFORMAT_ARGB8888 };
extern "C" SDL_Surface* SDL_CreateRGBSurfaceWithFormat(Uint32,int,int,int,Uint32);
extern "C" Uint32 SDL_MapRGBA(const SDL_PixelFormat*,Uint8,Uint8,Uint8,Uint8);
extern "C" void SDL_FreeSurface(SDL_Surface*);
extern "C" SDL_Texture* SDL_CreateTextureFromSurface(SDL_Renderer*,SDL_Surface*);

// Original HF93 SDL audio contract; DOS runtime backend pending.
enum { SDL_INIT_AUDIO=0x10, AUDIO_S16SYS=0x8010 };
extern "C" {
uint64_t SDL_GetPerformanceFrequency(void);
uint64_t SDL_GetPerformanceCounter(void);
int SDL_setenv(const char*,const char*,int);
Uint32 SDL_WasInit(Uint32);
int SDL_InitSubSystem(Uint32);
SDL_AudioDeviceID SDL_OpenAudioDevice(const char*,int,const SDL_AudioSpec*,SDL_AudioSpec*,int);
void SDL_PauseAudioDevice(SDL_AudioDeviceID,int);
const char* SDL_GetCurrentAudioDriver(void);
void SDL_CloseAudioDevice(SDL_AudioDeviceID);
}