#include "CommandSystem.h"
#include "LocalSimulation.h"
#include "OriginalData.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

namespace {
[[noreturn]] void fail(const std::string& s){
    std::cerr<<"M121 Drop/Ready popup FAIL: "<<s<<"\n";
    std::exit(1);
}
}

int main(){
    OriginalData original;
    LocalSimulation sim(original);

    LocalState castle;
    castle.active=true;
    castle.inTown=false;
    castle.tilesInitialized=true;
    castle.tiles.fill(1);
    castle.x=10;
    castle.y=10;

    PlayerSave p;
    p.coin=100;
    p.weapons.fill(0);
    p.weapons[0]=1;
    p.weapons[1]=2;
    p.weapons[7]=1;
    p.armour.fill(0);
    p.armour[0]=1;
    p.armour[2]=1;
    p.spells.fill(0);
    p.spells[1]=1;
    p.spells[10]=1;
    p.readyWeapon=1;
    p.readyArmour=0;
    p.readySpell=1;

    CommandSystem cmd;
    std::vector<std::string> out;

    // Drop category is a live command line, same model as Ready.
    if(cmd.beginDrop(castle,p,out)) fail("castle Drop completed at begin");
    if(!out.empty()) fail("Drop category was pushed as completed history");
    if(cmd.state()!=CommandSystem::State::DropCategory) fail("not DropCategory");
    if(cmd.inputLine()!="코인(P), 무기(W), 방어구(A) 버리기:  ")
        fail("Drop category translation/two-space gap");
    if(cmd.selectorPopupActive()) fail("Drop popup opened before P/W/A");

    // M122 oracle correction: P enters numeric input only; unlike W/A it does
    // NOT open the upper selector popup.
    out.clear();
    if(cmd.handleKey('P',castle,p,sim,out)) fail("P completed instead of entering amount input");
    if(cmd.state()!=CommandSystem::State::DropPence || cmd.dropPopupActive() ||
       cmd.selectorPopupActive())
        fail("Drop Pence incorrectly opened selector popup");
    if(cmd.inputLine()!="코인 던지기:  ") fail("Drop pence prompt");
    if(!cmd.dropMenuEntries(p).empty()) fail("Drop pence invented an alphabetic item list");
    if(cmd.readyCursorColumn()!=-1) fail("Drop pence cursor must follow measured prompt");

    // Numeric input remains visible on the live row.
    if(cmd.handleKey('1',castle,p,sim,out)) fail("Drop pence digit 1 completed");
    if(cmd.handleKey('2',castle,p,sim,out)) fail("Drop pence digit 2 completed");
    if(cmd.inputLine()!="코인 던지기:  12") fail("Drop pence live digits");
    out.clear();
    if(!cmd.handleKey('\r',castle,p,sim,out)) fail("Drop pence Enter did not finish");
    if(cmd.active()) fail("Drop pence remained active");
    if(p.coin!=88) fail("Drop pence amount not subtracted");

    // W opens owned-only list using original raw item letters.
    out.clear();
    cmd.beginDrop(castle,p,out);
    out.clear();
    if(cmd.handleKey('W',castle,p,sim,out)) fail("W completed instead of opening Drop weapon popup");
    if(cmd.state()!=CommandSystem::State::DropWeapon || !cmd.dropPopupActive())
        fail("Drop weapon popup state");
    if(cmd.inputLine()!="무기 버리기:  ") fail("Drop weapon prompt/two spaces");
    const auto weapons=cmd.dropMenuEntries(p);
    if(weapons.size()!=2 ||
       weapons[0]!=std::make_pair('b',std::string("단검")) ||
       weapons[1]!=std::make_pair('h',std::string("활과 화살")))
        fail("Drop weapon owned-only list");

    // A non-selection terminates with the approved "안 한다" and does not mutate.
    const auto daggerBefore=p.weapons[1];
    out.clear();
    if(!cmd.handleKey(' ',castle,p,sim,out)) fail("invalid Drop weapon key did not terminate");
    if(out.size()!=1 || out[0]!="안 한다") fail("Drop weapon invalid key != 안 한다");
    if(cmd.active()) fail("Drop weapon remained active after 안 한다");
    if(p.weapons[1]!=daggerBefore) fail("invalid Drop weapon key mutated inventory");

    // Valid weapon selection still drops one item.
    out.clear();
    cmd.beginDrop(castle,p,out);
    out.clear();
    cmd.handleKey('W',castle,p,sim,out);
    out.clear();
    if(!cmd.handleKey('b',castle,p,sim,out)) fail("valid Drop weapon did not finish");
    if(out.size()!=1 || out[0]!="단검") fail("Drop weapon selected-name output");
    if(p.weapons[1]!=daggerBefore-1) fail("Drop weapon did not decrement inventory");

    // A opens armour popup/list with the same title-frame policy.
    out.clear();
    cmd.beginDrop(castle,p,out);
    out.clear();
    if(cmd.handleKey('A',castle,p,sim,out)) fail("A completed instead of opening Drop armour popup");
    if(cmd.inputLine()!="방어구 버리기:  ") fail("Drop armour prompt/two spaces");
    const auto armour=cmd.dropMenuEntries(p);
    if(armour.size()!=1 || armour[0].first!='c')
        fail("Drop armour raw-index letters / non-droppable bare state");
    cmd.cancel();

    // Invalid Drop category choice also ends as "안 한다".
    out.clear();
    cmd.beginDrop(castle,p,out);
    out.clear();
    if(!cmd.handleKey('X',castle,p,sim,out)) fail("invalid Drop category did not finish");
    if(out.size()!=1 || out[0]!="안 한다") fail("Drop category invalid != 안 한다");

    // Ready item prompts also use two spaces and invalid/non-owned choices end.
    out.clear();
    cmd.beginReady(p,out);
    out.clear();
    if(cmd.handleKey('W',castle,p,sim,out)) fail("Ready W did not open popup");
    if(cmd.inputLine()!="무기 장비하기:  ") fail("Ready weapon prompt/two spaces");
    const auto readyBefore=p.readyWeapon;
    out.clear();
    if(!cmd.handleKey('\r',castle,p,sim,out)) fail("Ready non-alpha key did not finish");
    if(out.size()!=1 || out[0]!="안 한다") fail("Ready non-alpha != 안 한다");
    if(p.readyWeapon!=readyBefore) fail("Ready non-alpha changed equipped weapon");

    // An alphabetic key not represented in the visible owned list is also none.
    out.clear();
    cmd.beginReady(p,out);
    out.clear();
    cmd.handleKey('S',castle,p,sim,out);
    if(cmd.inputLine()!="주문 장비하기:  ") fail("Ready spell prompt/two spaces");
    out.clear();
    if(!cmd.handleKey('a',castle,p,sim,out)) fail("unowned Ready letter did not finish");
    if(out.size()!=1 || out[0]!="안 한다") fail("unowned Ready letter != 안 한다");

    // Armour prompt follows the same two-space rule.
    out.clear();
    cmd.beginReady(p,out);
    out.clear();
    cmd.handleKey('A',castle,p,sim,out);
    if(cmd.inputLine()!="방어구 장비하기:  ") fail("Ready armour prompt/two spaces");

    std::cout<<"M121 Drop/Ready popup + none + cursor-gap test OK\n";
    return 0;
}