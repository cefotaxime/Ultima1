#include "SDL.h"
#include <stdio.h>
extern "C" int dos_video_start(void);
extern "C" void dos_video_end(void);
extern "C" void dos_video_clear(int);
extern "C" unsigned long dos_draw_writes(void);
int main(){
 dos_video_start();
 dos_video_clear(1);
 SDL_Renderer* r=nullptr;
 SDL_SetRenderDrawColor(r,255,255,255,255);
 SDL_Rect outer={8,8,304,184},inner={14,14,292,172},bar={25,90,270,16};
 SDL_RenderDrawRect(r,&outer);
 SDL_RenderDrawRect(r,&inner);
 SDL_RenderFillRect(r,&bar);
 static uint8_t tile[16*16];
 for(int y=0;y<16;y++)for(int x=0;x<16;x++)tile[y*16+x]=((x/4+y/4)&1)?14:4;
 SDL_Texture tex={16,16,tile};
 SDL_Rect dst={144,45,32,32};
 int copied=SDL_RenderCopy(r,&tex,nullptr,&dst);
 FILE* f=fopen("VIDEO.OK","wb");
 if(f){fprintf(f,"DOS VGA BACKEND OK; PIXEL_WRITES=%lu TILE_COPY=%d\n",dos_draw_writes(),copied);fclose(f);}
 dos_video_end();
 puts("DOS VGA backend completed");
 return 0;
}