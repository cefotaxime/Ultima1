#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include <SDL.h>
#include "OriginalData.h"
#include "LocalSimulation.h"

class PixelFont;

struct SpaceCraft {
    int x=0;
    int y=0;
    int type=-1;       // SPACE.EXE record +4; -1 = 0xFFFF / unavailable
    int fuel=0;        // record +6
    int shield=0;      // record +8
    int heading=0;     // record +10, 0..3
    bool active() const { return type>=0; }
};

struct SpaceSector {
    int enemies=0;     // SPACE.EXE sector +0
    int starX=0;     // +2
    int starY=0;     // +4
    int baseX=0;       // +6
    int baseY=0;       // +8
    std::array<SpaceCraft,4> craft{};
};

struct SpaceState {
    bool active=false;
    bool frontView=false;      // SPACE.EXE DS:01F2 (0=top, 1=front)
    bool sectorScan=false;
    std::uint32_t sectorScanStartMs=0; // M250: initial left-to-right column reveal origin; blink follows reveal
    static constexpr std::uint32_t SECTOR_SCAN_COLUMN_MS=16; // one visible presentation quantum per original x-column
    bool docked=false;
    bool choosingShip=false;
    int sectorX=3;             // DS:01EE, initialized to 3 in the executable image
    int sectorY=3;             // DS:01F0
    int currentSlot=2;         // DS:5E14; Shuttle is home-sector record #2
    int velocityX=0;           // DS:0128, clamped -9..9
    int velocityY=0;           // DS:012A
    int thrustCounter=0;       // DS:012C; thrust-flame lifetime, +10 per Thrust, -1 per top tick
    int crosshairX=159;        // DS:013A
    int crosshairY=79;         // DS:013C
    int aimVelocityX=0;        // DS:0136, front-view aim step (-4/0/+4)
    int aimVelocityY=0;        // DS:0138
    int enemyX=0;              // DS:0C92
    int enemyY=0;              // DS:0C94
    int enemySprite=0;         // DS:0C9A, 0..6
    int enemyAdvanceCounter=0; // DS:0DE2

    // M18: SPACE.EXE front-flight presentation/state.
    int frontSpeed=1;          // DS:0126, numeric commands 1..8
    int frontEntrySpeed=1;     // DS:5E18, top/front velocity conversion baseline
    int starMode=2;            // DS:5FFE: 1=trail, 2=normal, 3=erase/move
    bool starsInitialized=false;
    std::array<int,40> starX{};      // DS:5E1A relative to reticle
    std::array<int,40> starY{};      // DS:5E6A
    std::array<int,40> starPrevX{};  // DS:5EBA logical screen coordinates
    std::array<int,40> starPrevY{};  // DS:5F0A
    std::vector<std::pair<int,int>> starTrail; // retained only for mode 1

    // HyperJump is blocking in DOS.  SDL runs the same phases incrementally.
    int hyperPhase=0;          // 0 idle, 1 accel, 2 trail, 3 erase, 4 decel/finalize
    int hyperTick=0;
    int hyperDelay=0;
    int hyperSpeedCursor=0;
    int hyperOriginalSpeed=1;
    int appleHyperVisualStage=0;
    int appleHyperVisualCalls=0;
    int hyperWaitRetraces=0; // M252: wait paid after the most recent phase-1/4 source update

    // Transient exact-geometry alien beam (DOS draws then erases after a delay).
    int alienShotFrames=0;
    int alienShotX0=0, alienShotY0=0, alienShotX1=0, alienShotY1=0;
    bool alienFireSfxPending=false;
    bool alienHitSfxPending=false;

    // M206: SPACE.EXE Fire draws two light-red beams from the lower corners
    // of the front viewport to the current gunsight, then erases them.
    std::uint32_t playerShotUntilMs=0;
    int playerShotX=159, playerShotY=79;
    bool playerHitSfxPending=false;

    int collisionFlashFrames=0;
    int collisionX=0, collisionY=0;
    bool crunchSfxPending=false; // SPACE.EXE 0x372B: push 0 -> sound(0) after "Crunch!"

    std::array<SpaceSector,49> sectors{};
};

class SpaceSystem {
public:
    bool loadGraphics(const std::string& dataDir, std::string& error);

    void enter(SpaceState& s, PlayerSave& p, std::vector<std::string>& messages);

    // Direction codes are the SPACE.EXE command codes: 1=left, 2=right,
    // 3=up, 4=down.  Their meaning depends on top/front view.
    void direction(SpaceState& s, PlayerSave& p, int code, std::vector<std::string>& messages);
    void toggleView(SpaceState& s, PlayerSave& p, std::vector<std::string>& messages);
    void scan(SpaceState& s, std::vector<std::string>& messages);
    void dismissScan(SpaceState& s);
    void hyperJump(SpaceState& s, PlayerSave& p, std::vector<std::string>& messages);
    void updateAppleHyperProgress(SpaceState& s,PlayerSave& p,std::vector<std::string>& messages,std::uint64_t progress);
    void setSpeed(SpaceState& s, PlayerSave& p, int speed, std::vector<std::string>& messages);
    void fire(SpaceState& s, PlayerSave& p, std::vector<std::string>& messages);
    void centerOrPass(SpaceState& s, std::vector<std::string>& messages);
    void invalidCommand(const char* text, std::vector<std::string>& messages) const;
    bool chooseShip(SpaceState& s, PlayerSave& p, int oneBasedPort,
                    std::vector<std::string>& messages);

    // Command completion no longer advances flight position: SPACE.EXE runs its
    // flight/target loop continuously while waiting for keyboard input.
    void updateAfterCommand(SpaceState& s, PlayerSave& p, std::vector<std::string>& messages);

    // One SPACE.EXE-style flight-loop tick.  The SDL main loop paces top view
    // at the original 2-BIOS-tick cadence and normal front view by the original
    // speed-indexed 8..1 vertical-retrace wait table recovered at M205.
    void updateFrame(SpaceState& s, PlayerSave& p, std::vector<std::string>& messages);

    void draw(SDL_Renderer* r, const PixelFont& font, const SpaceState& s,
              const PlayerSave& p, SDL_Color white) const;

    static int maxFuel(int shipType);
    static int maxShield(int shipType);
    static const char* shipTypeName(int shipType);

private:
    static int wrap7(int v);
    static int oppositeHeading(int h);
    static int portX(int baseX,int port,int shipType);
    static int portY(int baseY,int port,int shipType);
    static int clampVelocity(int v);
    static int sectorIndex(int x,int y) { return x*7+y; }

    SpaceSector& sector(SpaceState& s) const;
    const SpaceSector& sector(const SpaceState& s) const;
    SpaceCraft* currentCraft(SpaceState& s) const;
    const SpaceCraft* currentCraft(const SpaceState& s) const;

    void generateUniverse(SpaceState& s,const PlayerSave& p);
    void generateSector(SpaceSector& q,int sx,int sy);
    void buildHomeSector(SpaceState& s,const PlayerSave& p);
    void spawnEnemyTarget(SpaceState& s);
    bool isAtDock(const SpaceState& s,int slot) const;
    bool transferCraft(SpaceState& s,int nx,int ny,bool hyperJumpPlacement=false);
    void applyTopMotionTick(SpaceState& s,PlayerSave& p,std::vector<std::string>& messages);
    void applyFrontTick(SpaceState& s,PlayerSave& p,std::vector<std::string>& messages);
    void maybeAlienFire(SpaceState& s,PlayerSave& p,std::vector<std::string>& messages);
    void initStars(SpaceState& s);
    void initStar(SpaceState& s,int i);
    void updateStars(SpaceState& s);
    void updateHyperJump(SpaceState& s,PlayerSave& p,std::vector<std::string>& messages);
    static int roundDosFixed(int v);
    void checkLandingOrDock(SpaceState& s,PlayerSave& p,std::vector<std::string>& messages);
    void killPlayerInSpace(SpaceState& s,PlayerSave& p,std::vector<std::string>& messages,
                           bool shieldDrainedMessage=false);
    void applyCrunchDamage(SpaceState& s,PlayerSave& p,std::vector<std::string>& messages,
                           bool reverseVelocity);

    std::array<std::array<std::uint8_t,76>,12> topMasks_{};
    std::array<std::array<std::uint8_t,57>,7> fightMasks_{};
    std::array<std::uint16_t,18*19> starRaster_{};
    std::array<std::uint16_t,20*20> baseRaster_{};
    std::array<std::uint16_t,18*21> worldRaster_{};
    bool graphicsLoaded_=false;
    bool topSpritePixel(int type,int heading,int px,int py) const;
    bool fightSpritePixel(int stage,int px,int py) const;
    bool topCollision(const SpaceState& s,int movingSlot,int nx,int ny) const;
    void drawTopExact(SDL_Renderer* r,const SpaceState& s) const;
    void drawFightExact(SDL_Renderer* r,const SpaceState& s,SDL_Color white) const;
    void drawSpaceFrame(SDL_Renderer* r) const;
    void drawSectorScan(SDL_Renderer* r,const SpaceState& s) const;

    DosRng rng_;
};
