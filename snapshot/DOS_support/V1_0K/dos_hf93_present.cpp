#include "SDL.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
// HF93 ARGB frame presentation. VESA mode 0x101 640x480, 4:3 display.
extern "C" uint32_t* hf93TargetPixels(void);
extern "C" int hf93TargetWidth(void);
extern "C" int hf93TargetHeight(void);
extern "C" int hf93_commit_indexed(const unsigned char*);
static int paletteIndexSlow(uint32_t p){
 static const uint32_t ega[16]={0x000000,0x0000AA,0x00AA00,0x00AAAA,0xAA0000,0xAA00AA,0xAA5500,0xAAAAAA,0x555555,0x5555FF,0x55FF55,0x55FFFF,0xFF5555,0xFF55FF,0xFFFF55,0xFFFFFF};
 int best=0;unsigned bestD=~0u;
 for(int i=0;i<16;i++){
  int dr=(int)((p>>16)&255)-(int)((ega[i]>>16)&255);
  int dg=(int)((p>>8)&255)-(int)((ega[i]>>8)&255);
  int db=(int)(p&255)-(int)(ega[i]&255);
  unsigned dd=(unsigned)(dr*dr+dg*dg+db*db);
  if(dd<bestD){bestD=dd;best=i;}
 }
 return best;
}
static unsigned char paletteLut[32768];
static bool paletteLutReady=false;
static void initPaletteLut(){if(paletteLutReady)return;for(int v=0;v<32768;v++){const uint32_t rgb=((uint32_t)((v>>10)&31)*255/31<<16)|((uint32_t)((v>>5)&31)*255/31<<8)|((uint32_t)(v&31)*255/31);paletteLut[v]=(unsigned char)paletteIndexSlow(rgb);}paletteLutReady=true;}
static unsigned char paletteIndex(uint32_t p){return paletteLut[(((p>>19)&31)<<10)|(((p>>11)&31)<<5)|((p>>3)&31)];}
extern "C" int hf93PresentARGB(const uint32_t* argb,int w,int h){
 static int calls=0;
 if(calls<3){FILE* f=fopen("PRESENT.LOG","a");if(f){fprintf(f,"present call=%d ptr=%p size=%dx%d first=%08lx\n",++calls,(const void*)argb,w,h,argb?(unsigned long)*argb:0UL);fclose(f);}}
 if(!argb||w!=640||h!=400)return -1;
 initPaletteLut();
 // HF93 keepAspect projects 640x400 onto a 4:3 640x480 CRT raster.
 static unsigned char* indexed=nullptr;
 if(!indexed)indexed=(unsigned char*)malloc(640u*480u);
 if(!indexed)return -1;
 for(int y=0;y<480;y++)for(int x=0;x<640;x++)indexed[y*640+x]=(unsigned char)paletteIndex(argb[(y*400/480)*640+x]);
 int status=hf93_commit_indexed(indexed);
 return status;
}