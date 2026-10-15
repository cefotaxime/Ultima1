#pragma once
#include <string>
#include <vector>
#include "OriginalData.h"
#include "LocalSimulation.h"

class ResurrectionSystem {
public:
    ResurrectionSystem(const OriginalData& original, LocalSimulation& sharedRng)
        : original_(original), rng_(sharedRng) {}

    bool needed(const PlayerSave& p) const { return p.hits==0 || p.food==0; }

    // Animated OUT.EXE-compatible resurrection phases.  main.cpp owns the
    // presentation clock; these methods perform only the state mutations that
    // occur at each original phase boundary.
    void beginAnimated(PlayerSave& p) const;
    void afterHitsAnimated(PlayerSave& p) const;
    void finishAnimated(PlayerSave& p) const;

    // Immediate compatibility path retained for headless/historical callers.
    std::vector<std::string> resurrect(PlayerSave& p) const;

private:
    static int quadrant(int x,int y){ return (y>77?2:0)+(x>83?1:0); }
    bool occupied(const PlayerSave& p,int x,int y) const;
    void removeAllMonsters(PlayerSave& p) const;
    void finishCommon(PlayerSave& p) const;

    const OriginalData& original_;
    LocalSimulation& rng_;
};