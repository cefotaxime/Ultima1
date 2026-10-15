#pragma once
#include <SDL.h>
#include <array>
#include <cstdint>
#include <string>
#include "OriginalData.h"
#include "PixelFont.h"

enum class FrontendAction { None, StartGame, SaveCharacter, Quit };

enum class FrontendScreen { IntroCopyright, IntroPresents, IntroRelease, IntroMasterpiece, IntroCredits, Title, Menu, Replacement, Selection, Loading, InsertDisk, Attributes, Race, Sex, Class, Name, Confirm };

class FrontendSystem {
public:
    FrontendSystem();
    bool active() const { return active_; }
    void dismiss() { active_=false; }
    void restartAtOpening() {
        active_=true; enterGamePending_=false; introSkipPending_=false;
        forceScreen("copyright");
    }
    FrontendScreen screen() const { return screen_; }
    void forceScreen(const std::string& name);
    void update(std::uint32_t now);
    void setRoster(const std::array<bool,4>& occupied,const std::array<std::string,4>& names);
    FrontendAction handleKey(SDL_Keycode key, SDL_Keymod mod, PlayerSave& out, bool haveSave);
    void draw(SDL_Renderer* r, const PixelFont& font, const OriginalData& original, bool haveSave) const;
    int selectedSlot() const { return selectedSlot_; }
    int generationSlot() const { return generationSlot_; }
    bool enterGamePending() const { return enterGamePending_; }
    void finishLoading() { enterGamePending_=false; active_=false; }

private:
    bool active_=true;
    FrontendScreen screen_=FrontendScreen::IntroCopyright;
    std::uint32_t introStageStart_=0;
    bool introSkipPending_=false;
    int titleFlagLeft_=1;
    int titleFlagRight_=1;
    std::uint32_t titleFlagStep_=0;
    std::uint32_t titleFlagRng_=0x554C5449u; // deterministic host PRNG state; only animation appearance
    std::array<int,6> stats_{{10,10,10,10,10,10}};
    int pointsLeft_=30;
    int selectedStat_=0;
    int race_=0, sex_=0, klass_=0;
    std::string name_;
    enum class NameInputMode { Korean, English };
    NameInputMode nameInputMode_=NameInputMode::Korean;
    int hangulL_=-1, hangulV_=-1, hangulT_=0;
    std::string notice_;
    std::array<bool,4> occupied_{{false,false,false,false}};
    std::array<std::string,4> rosterNames_{{"","","",""}};
    int selectedSlot_=-1;
    int generationSlot_=0;
    bool enterGamePending_=false;

    void beginGeneration();
    void applyRace(int choice);
    void applyClass(int choice);
    PlayerSave buildPlayer() const;
    std::string displayName() const;
    void clearHangulComposition();
    void commitHangulComposition();
    void backspaceName();
    void inputKoreanKey(SDL_Keycode key, SDL_Keymod mod);
    void inputEnglishKey(SDL_Keycode key, SDL_Keymod mod);
    void toggleNameInputMode();
    static std::size_t utf8Length(std::string_view s);
    static const char* raceName(int v);
    static const char* sexName(int v);
    static const char* className(int v);
};