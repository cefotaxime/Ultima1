#include "SDL.h"
#include <stdio.h>
#include <stdint.h>
extern "C" int dos_video_start();
extern "C" void dos_video_end();
extern "C" void dos_video_clear(int);
extern "C" unsigned long dos_draw_writes();
int main(){
 FILE*f=fopen("EGATILES.BIN","rb");
 if(!f){puts("EGATILES.BIN missing");return 1;}
 uint8_t raw[6656];
 size_t got=fread(raw,1,sizeof raw,f);fclose(f);
 if(got!=sizeof raw){puts("EGATILES.BIN invalid size");return 2;}
 dos_video_start();dos_video_clear(1);
 SDL_Renderer*r=nullptr;
 for(int tile=0;tile<52;tile++){
  uint8_t pixels[256];
  for(int y=0;y<16;y++)for(int x=0;x<16;x++){
   int index=0;
   for(int plane=0;plane<4;plane++)if(raw[tile*128+y*8+plane*2+x/8]&(128>>(x%8)))index|=1<<plane;
   pixels[y*16+x]=(uint8_t)index;
  }
  SDL_Texture tex{16,16,pixels};
  SDL_Rect dest{16+(tile%13)*22,18+(tile/13)*42,16,16};
  if(SDL_RenderCopy(r,&tex,nullptr,&dest)!=0){dos_video_end();return 3;}
 }
 FILE*out=fopen("TILES.OK","wb");
 if(out){fprintf(out,"EGA 52 tiles decoded from 6656 bytes; pixels=%lu\n",dos_draw_writes());fclose(out);}
 dos_video_end();
 puts("EGA tile decode and VGA copy complete");
 return 0;
}