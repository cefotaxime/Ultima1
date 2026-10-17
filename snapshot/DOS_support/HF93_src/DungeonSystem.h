#pragma once
#include <SDL.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "OriginalData.h"
#include "LocalSimulation.h"

struct DungeonCell {
    std::uint16_t feature=0;       // OUT.EXE DS:AD56 + cell*8
    std::uint16_t monster=0xFFFF;  // +2; FFFF = empty
    std::uint16_t marker=0;        // +4; 4=Chest, 5=Coffin, +1000 temporary move flag
    std::uint16_t hp=0;            // +6; monster HP/value
};

struct DungeonState {
    bool active=false;
    int locationId=0;
    int x=1,y=1;
    int level=1;
    int orientation=4; // 1=West, 2=East, 3=North, 4=South
    int exitHitBonus=0; // OUT.EXE DS:AD54; combat will populate this in M9.
    std::array<DungeonCell,121> cells{};
};

class DungeonSystem {
public:
    DungeonSystem(const OriginalData& original, LocalSimulation& sharedRng)
        : original_(original), rng_(sharedRng) {}

    void enter(DungeonState& d,const PlayerSave& p,int locationId,std::vector<std::string>& messages);
    void regenerate(DungeonState& d,const PlayerSave& p);
    void turnAround(DungeonState& d,std::vector<std::string>& messages) const;
    void turnLeft(DungeonState& d,std::vector<std::string>& messages) const;
    void turnRight(DungeonState& d,std::vector<std::string>& messages) const;
    void forward(DungeonState& d,std::vector<std::string>& messages) const;
    void searchSecret(DungeonState& d,std::vector<std::string>& messages) const;
    void openCoffin(DungeonState& d,PlayerSave& p,std::vector<std::string>& messages);
    void unlockChest(DungeonState& d,PlayerSave& p,std::vector<std::string>& messages);
    // Returns true when climbing level 1 upward exits to the overworld.
    bool climb(DungeonState& d,PlayerSave& p,std::vector<std::string>& messages);

    // M9: exact OUT.EXE dungeon combat/update core.
    void attack(DungeonState& d,PlayerSave& p,std::vector<std::string>& messages);
    // M187: source-exact OUT.EXE dungeon Cast dispatcher (raw 0x15A8).
    // Spell 0 is Prayer and randomly invokes one of the real dungeon effects;
    // real spells 1..10 use the player's current facing where direction matters.
    void castDungeon(DungeonState& d,PlayerSave& p,int spell,std::vector<std::string>& messages);
    int lastCastEffectiveSpell() const { return lastCastEffectiveSpell_; }
    bool lastCastFailed() const { return lastCastFailed_; }
    bool lastCastHit() const { return lastCastHit_; }
    void updateAfterCommand(DungeonState& d,PlayerSave& p,std::vector<std::string>& messages);
    // M174: OUT.EXE dungeon renderer emits visible center-line entity names
    // into the command window without a command prompt while redrawing.
    std::vector<std::string> visibleObjectMessages(const DungeonState& d) const;
    static const char* monsterName(int id);

    void drawWireframe(SDL_Renderer* renderer,const DungeonState& d,SDL_Color color) const;
    const DungeonCell& cell(const DungeonState& d,int x,int y) const;
    static const char* orientationName(int o);

private:
    DungeonCell& cell(DungeonState& d,int x,int y) const;
    static std::uint16_t dosTimeHash();
    void generate(DungeonState& d,const PlayerSave& p);
    void placeMonster(DungeonState& d);
    bool spawnMonsterAt(DungeonState& d,int x,int y);
    bool completeMonsterQuest(int monsterId,PlayerSave& p,std::vector<std::string>& messages) const;
    void drawMonsterVector(SDL_Renderer* renderer,int monsterId,int depth,SDL_Color color) const;
    void drawDungeonVector(SDL_Renderer* renderer,int vectorId,int depth,SDL_Color color) const;
    void drawExactBars(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawFrontFace(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawSecretFrontFace(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawLeftWallSegment(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawRightWallSegment(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawLeftOpenSegment(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawRightOpenSegment(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawLeftSecretSegment(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawRightSecretSegment(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawLeftByFeature(SDL_Renderer* renderer,std::uint16_t feature,int depth,SDL_Color color) const;
    void drawRightByFeature(SDL_Renderer* renderer,std::uint16_t feature,int depth,SDL_Color color) const;
    void drawLeftTransition(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    void drawRightTransition(SDL_Renderer* renderer,int depth,SDL_Color color) const;
    bool dungeonSightBlocker(const DungeonState& d,int x,int y) const;
    int sightDepth(const DungeonState& d,int fx,int fy) const;
    static int sideClass(std::uint16_t feature);
    bool moveMonster(DungeonState& d,int x,int y,int dx,int dy);
    void monsterAttack(DungeonState& d,PlayerSave& p,int monsterId,std::vector<std::string>& messages);
    static bool blocksMove(const DungeonCell& from,const DungeonCell& to);
    static bool wallLike(std::uint16_t f){ return f==1 || f==2; }
    static bool blocksSight(std::uint16_t f){ return f==1 || f==2 || f==3; }
    void delta(int orientation,int& dx,int& dy) const;
    void leftDelta(int orientation,int& dx,int& dy) const;

    const OriginalData& original_;
    LocalSimulation& rng_;
    int lastCastEffectiveSpell_=0;
    bool lastCastFailed_=false;
    bool lastCastHit_=false;
};
