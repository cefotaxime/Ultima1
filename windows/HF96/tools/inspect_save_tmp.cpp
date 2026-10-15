#include <iostream>
#include "OriginalData.h"
int main(int argc,char**argv){
  if(argc<2) return 2;
  OriginalData o; std::string e; PlayerSave p;
  if(!o.loadSave(argv[1],p,e)){ std::cerr<<e<<"\n"; return 1; }
  std::cout<<"hits="<<p.hits<<" coin="<<p.coin<<" food="<<p.food<<" exp="<<p.experience
           <<" str="<<p.strength<<" agi="<<p.agility<<" sta="<<p.stamina
           <<" race="<<(int)p.race<<" sex="<<(int)p.sex<<" class="<<(int)p.klass
           <<" readyW="<<p.readyWeapon<<" readyA="<<p.readyArmour<<"\n";
  return 0;
}