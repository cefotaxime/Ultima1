#pragma once
#include <string>
#include <utility>
#include <vector>
#include "OriginalData.h"
#include "LocalSimulation.h"

class CommandSystem {
public:
    enum class State { None, ReadyCategory, ReadyWeapon, ReadyArmour, ReadySpell,
                       DropCategory, DropWeapon, DropArmour, DropPence };
    bool active() const { return state_ != State::None; }
    State state() const { return state_; }
    bool readyPopupActive() const {
        return state_==State::ReadyWeapon || state_==State::ReadyArmour || state_==State::ReadySpell;
    }
    bool dropPopupActive() const {
        // Original Drop Pence stays on the command line; only W/A use the
        // upper inventory selector popup.
        return state_==State::DropWeapon || state_==State::DropArmour;
    }
    bool selectorPopupActive() const { return readyPopupActive() || dropPopupActive(); }

    std::string inputLine() const;
    int readyCursorColumn() const;
    std::vector<std::pair<char,std::string>> readyMenuEntries(const PlayerSave& p) const;
    std::vector<std::pair<char,std::string>> dropMenuEntries(const PlayerSave& p) const;

    // Return true when the command completes on this call.
    bool beginReady(PlayerSave& p,std::vector<std::string>& out);
    bool beginDrop(const LocalState& local,PlayerSave& p,std::vector<std::string>& out);
    bool handleKey(char ch,LocalState& local,PlayerSave& p,LocalSimulation& sim,
                   std::vector<std::string>& out);
    void cancel(){ state_=State::None; digits_.clear(); }

private:
    State state_=State::None;
    std::string digits_;
    static int itemIndex(char ch,int maxIndex);
    static const char* weaponName(int i);
    static const char* armourName(int i);
    static const char* spellName(int i);
};