#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>
#include "OriginalData.h"

struct LocalState {
    bool active=false;
    bool inTown=false;
    int locationId=0;
    int mapIndex=-1;
    int x=0,y=0;
    bool alert=false;       // OUT.EXE DS:AD46. Set by hostile/illegal acts; resets on local entry.
    bool foundKey=false;    // runtime flag corresponding to OUT.EXE DS:8BA0 for Jester key event.
    int drinks=0;           // OUT.EXE DS:AD48; reset on each local entry.
    std::uint16_t correctKeyTile=0; // OUT.EXE DS:AD4E; castle key type 0x3C/0x3D.
    std::array<std::uint8_t,OriginalData::LOCAL_W*OriginalData::LOCAL_H> tiles{};
    bool tilesInitialized=false;
    std::array<LocalActor,OriginalData::LOCAL_ACTOR_COUNT> actors{};
};

class DosRng {
public:
    void seedFromDosTime(std::uint16_t saveSeedWord);
    void seed(std::uint16_t value) { state_=value; }
    std::uint16_t state() const { return state_; }
    int range(int minValue,int maxValue);
private:
    std::uint16_t state_=1;
};

class LocalSimulation {
public:
    explicit LocalSimulation(const OriginalData& original):original_(original){}

    void seedFromDosTime(std::uint16_t saveSeedWord){rng_.seedFromDosTime(saveSeedWord);}
    void seed(std::uint16_t value){rng_.seed(value);}
    std::uint16_t rngState() const {return rng_.state();}
    int randomRange(int minValue,int maxValue){ return rng_.range(minValue,maxValue); }

    // Exact OUT.EXE-derived actor update layer (load-image 0x252C..0x2B6C).
    std::vector<std::string> updateAfterCommand(LocalState& local, PlayerSave& player);

    // M124 presentation layer: random Bard/Jester speech is generated during
    // the same actor update (and therefore consumes exactly the same RNG), but
    // the host may present it after a short delay. This drains the current
    // turn's random-dialogue rows.
    std::vector<std::string> takeDelayedActorDialogue();

    // M217: number of successful local-actor moves in the just-completed
    // actor update.  Original OUT.EXE emits sound(4) after each such move.
    int takeMovementSoundCount(){ const int n=movementSoundCount_; movementSoundCount_=0; return n; }
    std::vector<bool> takeGuardAttackHits(){ auto v=guardAttackHits_; guardAttackHits_.clear(); return v; }

    // Exact local actor attack formulas/targeting used by the normal weapon command.
    std::vector<std::string> attack(LocalState& local, PlayerSave& player, int dx, int dy);
    // OUT.EXE 0x2B6D local attack uses sound(2) only on a successful hit.
    bool lastPlayerAttackHit() const { return lastPlayerAttackHit_; }

    // M19 command handlers recovered from OUT.EXE 0x5D8A..0x604E and 0x7E31..0x7F6A.
    std::vector<std::string> steal(LocalState& local, PlayerSave& player);
    std::vector<std::string> unlock(LocalState& local, PlayerSave& player);
    // M20: OUT.EXE 0x5750/0x56FB — castle storeroom Get command.
    // allowance is the unsaved runtime word DS:8BA4, set to 9 by the fourth gem reward.
    std::vector<std::string> get(LocalState& local, PlayerSave& player, int& allowance);
    // M21: OUT.EXE 0x1CE4/0x1DC0 local donation effects after Pence is removed.
    std::vector<std::string> dropPence(LocalState& local,PlayerSave& player,int amount);
    // M22: OUT.EXE 0x269D + 0x4DD6. Called only when leaving a castle map.
    bool princessAdjacent(const LocalState& local) const;
    std::vector<std::string> rescuePrincess(LocalState& local,PlayerSave& player);
    std::uint8_t rawTile(const LocalState& local,int x,int y) const;
    int displayTile(const LocalState& local,int x,int y) const;
    bool passable(const LocalState& local,int x,int y) const;

    const LocalActor* actorAt(const LocalState& local,int x,int y) const;

private:
    int actorIndexAt(const LocalState& local,int x,int y) const;
    int queryCell(const LocalState& local,int x,int y) const;
    bool tryMoveActor(LocalState& local,int actorIndex,int dx,int dy,bool allowExitTile);
    int randomStep();
    void peacefulWander(LocalState& local,int actorIndex);
    void bardOrJester(LocalState& local,PlayerSave& player,int actorIndex,std::vector<std::string>& out);
    void princess(LocalState& local,int actorIndex);
    bool guardChase(LocalState& local,int actorIndex);
    void guardAttack(LocalState& local,PlayerSave& player,int actorIndex,std::vector<std::string>& out);

    const OriginalData& original_;
    DosRng rng_;
    std::vector<std::string> delayedActorDialogue_;
    bool lastPlayerAttackHit_=false;
    int movementSoundCount_=0;
    std::vector<bool> guardAttackHits_;
};