#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "OriginalData.h"
#include "LocalSimulation.h"
#include "PixelFont.h"

struct MondainState {
    bool active=false;
    bool gemDestroyed=false;
    bool victorious=false;
    bool mondainDead=false;
    bool defeated=false;             // M260: command-line death notice + framed defeat popup
    bool victoryPopup=false;          // M260: framed THOU ART VICTORIOUS stage
    // M273: original victory routine 0x1EA8 first blits corpse slot 4, then
    // builds the popup, then runs sound(5)x3.  SDL exposes those source-order
    // draw operations over successive presents without adding a timed delay.
    std::uint8_t victoryDrawPhase=0;   // 0=corpse/death row, 1=popup
    // Victory popup must reach the display before its three synchronous
    // PositiveEffect BEEPs.  The Apple-II-inspired ending transition begins
    // one second after the third BEEP returns: random-pixel dissolve-out,
    // then bottom-to-top scrolling Korean ending text.
    bool victoryPopupPresented=false;
    std::uint32_t victoryPopupPresentedMs=0;
    bool endingDissolveActive=false;
    std::uint32_t endingDissolveStartedMs=0;
    bool endingActive=false;
    bool endingScrollStarted=false;
    std::uint32_t endingScrollStartedMs=0;
    int endingScrollOffset=0;
    bool endingTextComplete=false;
    std::uint32_t endingTextCompletedMs=0;
    bool endingRestartLineVisible=false;
    bool engaged=false;              // MONDAIN DS:0246: set by attack/gem contact
    bool introActive=false;          // M258: four-page Time Machine narration is blocking
    std::uint8_t introPage=0;         // MONDAIN.EXE 0x1DE6: page 0..3, then final encounter
    // Presentation gate only: 0 means the newly selected page has not yet
    // reached SDL_RenderPresent; 1 means the complete page+wait prompt is visible
    // and MONDAIN.EXE-equivalent Space/Return polling may accept input.
    std::uint8_t introDrawPhase=0;
    int playerX=3, playerY=4;
    int mondainX=15, mondainY=4;
    int mondainHp=1000;
    int gemX=14, gemY=4;             // initial evil gem is arena tile 6
    int gemPulse=0, gemPulseDir=1;   // MONDAIN DS:0190/0192 visual pulse
    int transientStage=-1;           // MONDAIN DS:0130, -1 inactive, 0..6 effect animation
    int transientX=-1, transientY=-1;
    int mondainSprite=2;             // MONDAIN DS:0240
    int mondainAnim=0;               // MONDAIN DS:0242
    int mondainAnimLimit=20;         // MONDAIN DS:0244
    // M263 presentation/audio latches.  These are set by the source-level
    // final-arena routines and consumed only after the corresponding frame has
    // reached the host display, matching the DOS draw-then-sound ordering.
    bool playerAttackHitSfxPending=false;
    bool playerSpellSfxPending=false;      // successful final-arena spell -> sound(5)
    bool playerSpellHitSfxPending=false;   // Magic Missile hit -> sound(2) after sound(5)
    bool playerSpellFailSfxPending=false;  // failed/no-effect final-arena spell -> sound(6)
    bool enemyAttackSfxPending=false;   // melee sound(3)
    bool enemySpellSfxPending=false;    // spell sound(5)
    bool enemyHitSfxPending=false;
    bool psionicHitAfterRestorePending=false;
    bool psionicInvertPending=false;
    bool gemDestroySfxPending=false;
    bool victoryPopupSfxPending=false; // M271: original victory popup sound(5) x3 after presentation
    std::array<std::uint8_t,19*9> arena{};
};

class MondainSystem {
public:
    explicit MondainSystem(const OriginalData& original):original_(original){}
    void enter(MondainState& s, PlayerSave& p, std::vector<std::string>& out) const;
    bool move(MondainState& s,int dx,int dy,std::vector<std::string>& out) const;
    bool getGem(MondainState& s,PlayerSave& p,std::vector<std::string>& out) const;
    bool castSpell(MondainState& s,PlayerSave& p,LocalSimulation& sim,int spellId,int direction,std::vector<std::string>& out) const;
    void inform(const MondainState& s,std::vector<std::string>& out) const;
    void placeGemForOracle(MondainState& s,int x,int y) const;
    void damageMondain(MondainState& s,int damage,std::vector<std::string>& out) const;
    bool attack(MondainState& s,PlayerSave& p,LocalSimulation& sim,int direction,std::vector<std::string>& out) const;
    void updateAfterCommand(MondainState& s,PlayerSave& p,LocalSimulation& sim,std::vector<std::string>& out) const;
    // MONDAIN.EXE 0x05DB calls 0x1477 once per keyboard-poll loop; when no key
    // is pending the source then waits 3 BIOS ticks before polling again.  Keep
    // gem/fire/Mondain visual animation on that input-wait cadence, not on turns.
    void animateInputWaitTick(MondainState& s,LocalSimulation& sim) const;
    void draw(SDL_Renderer* r,const PixelFont& font,const MondainState& s) const;
    void updateEndingPresentation(MondainState& s,std::uint32_t nowMs) const;
    void acknowledgeIntro(MondainState& s,std::vector<std::string>& out) const {
        // MONDAIN.EXE image 0x1DE6 presents four framed narration pages,
        // waiting for Space/Return after each one.  Only after page 4 is
        // dismissed does the final encounter renderer take ownership.
        (void)out;
        if(!s.introActive) return;
        if(s.introPage<3){ ++s.introPage; s.introDrawPhase=0; return; }
        s.introActive=false; s.introDrawPhase=0;
    }
    void acknowledgeDefeat(MondainState& s) const { if(s.defeated) s.active=false; }
    void acknowledgeVictory(MondainState& s,std::uint32_t nowMs) const {
        if(s.victoryPopup && s.victoryDrawPhase>=1 && !s.victoryPopupSfxPending){
            s.endingDissolveActive=true;
            s.endingDissolveStartedMs=nowMs;
            s.endingActive=false;
            s.endingScrollStarted=false; s.endingScrollStartedMs=0;
            s.endingScrollOffset=0; s.endingTextComplete=false;
            s.endingTextCompletedMs=0; s.endingRestartLineVisible=false;
        }
    }
    static bool adjacent8(int ax,int ay,int bx,int by);
private:
    static bool inside(int x,int y){return x>=0&&x<19&&y>=0&&y<9;}
    static std::size_t idx(int x,int y){return static_cast<std::size_t>(x*9+y);} // MONDAIN x-major
    bool tryMoveMondain(MondainState& s,int dx,int dy) const;
    void engage(MondainState& s) const;
    void hurtPlayer(PlayerSave& p,int damage,std::vector<std::string>& out) const;
    void melee(MondainState& s,PlayerSave& p,LocalSimulation& sim,std::vector<std::string>& out) const;
    void cast(MondainState& s,PlayerSave& p,LocalSimulation& sim,std::vector<std::string>& out) const;
    const OriginalData& original_;
};