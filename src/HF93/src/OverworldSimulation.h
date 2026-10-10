
#pragma once
#include <cstdint>
#include <string>
#include <utility>
#include <vector>
#include "OriginalData.h"
#include "LocalSimulation.h"

enum class WorldAttackMode { Normal, Fire, MagicMissile, KillSpell };

struct WorldEnemyAttackEvent {
    // Absolute wrapped world cells where OUT.EXE 0x83FC draws physical tile 50.
    // The mountain/player terminal cell is included because the original blits
    // the effect before testing whether the ray should stop there.
    std::vector<std::pair<int,int>> path;
    bool hit=false;
    int damage=0;

    // Original prints "<monster> attacks!" before the projectile, then appends
    // Hit/Missed after it.  Korean keeps one logical command-window stream.
    std::string attackPrefix;
    std::string resultSuffix;
};

class OverworldSimulation {
public:
    OverworldSimulation(const OriginalData& original, LocalSimulation& sharedRng)
        : original_(original), sharedRng_(sharedRng) {}

    // OUT.EXE central command loop calls monster spawn before the step counter
    // is incremented, then the monster update dispatcher afterwards.
    void maybeSpawn(PlayerSave& player);
    std::vector<std::string> updateAfterCommand(PlayerSave& player);

    // M140 presentation oracle: OUT.EXE 0x83FC fixed eight-direction enemy projectile paths.
    // updateAfterCommand() refreshes this list each call; the SDL host drains it
    // immediately so logical simulation and visual presentation stay separate.
    std::vector<WorldEnemyAttackEvent> takeEnemyAttackEvents();

    // Called immediately after a successful overworld move. OUT.EXE removes
    // monsters from the quadrant the player just left when a quadrant boundary
    // is crossed.
    void onPlayerMoved(PlayerSave& player, int oldX, int oldY);

    // Normal Attack and Frigate/Aircar Fire share OUT.EXE's common world target
    // scan and kill/reward path.
    std::vector<std::string> attack(PlayerSave& player, int dx, int dy, WorldAttackMode mode);
    // OUT.EXE player-attack presentation: sound(2) is emitted only after a
    // successful player hit. The SDL host reads this after attack() so it can
    // preserve the source routine's synchronous SFX hold before the enemy turn.
    bool lastPlayerAttackHit() const { return lastPlayerAttackHit_; }

    const WorldObject* monsterAt(const PlayerSave& player, int x, int y) const;
    static bool isMonsterTile(int tile);
    static const char* monsterName(int tile);
    static int monsterPower(int tile);
    static int monsterRange(int tile);

private:
    int lastObject(const PlayerSave& p) const;
    int objectAt(const PlayerSave& p, int x, int y, int startIndex) const;
    int queryCell(const PlayerSave& p, int x, int y, int startIndex) const;
    int monsterCount(const PlayerSave& p) const;
    bool appendObject(PlayerSave& p, const WorldObject& o) const;
    void eraseObject(PlayerSave& p, int index) const;

    static int wrapX(int x);
    static int wrapY(int y);
    static int quadrant(int x, int y);
    static bool seamXOk(int x);
    static bool seamYOk(int y);
    static int sign(int v);

    bool tryMonsterAxis(PlayerSave& p, int objectIndex, bool yAxis, int dx, int dy);
    bool monsterMayEnter(PlayerSave& p, int objectIndex, int targetX, int targetY);
    void moveMonster(PlayerSave& p, int objectIndex, int dx, int dy);
    void monsterAttack(PlayerSave& p, int objectIndex, int dx, int dy, int range,
                       std::vector<std::string>& messages);
    void cleanupOldQuadrant(PlayerSave& p, int oldQuadrant);

    std::uint16_t cRandNext();
    std::string addCoinReward(PlayerSave& p, int amount) const;

    const OriginalData& original_;
    LocalSimulation& sharedRng_; // DOS random routine at OUT.EXE 0xB4C3 is global/shared.
    std::uint16_t cRandState_=1; // OUT.EXE DS:17A4 initial word; 0x91D4 runtime rand().
    std::vector<WorldEnemyAttackEvent> enemyAttackEvents_;
    bool lastPlayerAttackHit_=false;
};
