#pragma once
#include <SDL.h>
#include <array>
#include <cstdint>
#include <string>
#include <vector>

struct WorldObject {
    std::array<std::uint16_t,8> w{}; // exact 16-byte PLAYER*.U1 record
    std::uint16_t tile() const { return w[0]; }
    std::uint16_t terrainClass() const { return w[1]; }
    std::uint16_t x() const { return w[2]; }
    std::uint16_t y() const { return w[3]; }
    bool empty() const {
        for(auto v:w) if(v!=0) return false;
        return true;
    }
};

struct PlayerSave {
    std::string name;
    std::uint16_t race=0, klass=0, sex=0;
    std::uint16_t hits=0, strength=0, agility=0, stamina=0, charisma=0, wisdom=0, intelligence=0;
    std::uint16_t coin=0, experience=0, food=0;
    std::uint16_t readyWeapon=0, readySpell=0, readyArmour=0, transport=0;
    std::uint16_t rngSeedWord=0;
    std::uint16_t x=0, y=0;

    // OUT.EXE indexes DS:3AB2 + castleIndex*2, where PLAYER save base is DS:3A78.
    // index 1..8 are the eight kings: 0=none, 0xFFFF=active, 1=completed.
    std::array<std::uint16_t,9> kingQuest{};
    // PLAYER +0x4C,+0x4E,+0x50,+0x52; rewarded by the even-numbered kings.
    std::array<std::uint16_t,4> gems{}; // red, green, blue, white

    std::array<std::uint16_t,6> armour{};
    std::array<std::uint16_t,16> weapons{};
    std::array<std::uint16_t,11> spells{};
    std::array<std::uint16_t,8> transports{};
    std::uint32_t steps=0;
    // PLAYER +0xA6. SPACE.EXE increments this on every enemy-craft kill and
    // awards Space Ace exactly when the value reaches 20.
    std::uint16_t spaceKills=0;
    std::uint16_t shuttleStat0=0; // PLAYER +0xB0; SPACE.EXE persists Shuttle fuel here on landing.
    std::uint16_t shuttleStat1=0; // PLAYER +0xB2; OUT.EXE sets to 1000 when Shuttle is bought.
    std::array<WorldObject,40> worldObjects{}; // PLAYER +0xB4 to EOF, 40*16 bytes
};

enum class LocationKind { Town, Castle, Landmark, Dungeon };

struct LocationInfo {
    int id=0;
    int x=0, y=0;
    std::string name;
    LocationKind kind=LocationKind::Town;
    bool hasLocalMap() const { return kind==LocationKind::Town || kind==LocationKind::Castle; }
};

struct DungeonVectorCommand {
    bool move=false; // OUT.EXE 0x7E = move-to; ordinary entries are line-to.
    std::int8_t x=0;
    std::int8_t y=0;
};

struct LocalActor {
    std::uint16_t tile=0xFFFF;
    std::uint16_t x=0;
    std::uint16_t y=0;
    std::uint16_t value=0;
    bool active() const { return tile != 0xFFFF; }
};

class OriginalData {
public:
    bool load(const std::string& dataDir, SDL_Renderer* renderer, std::string& error);
    // M33 headless campaign audit loader.  It parses every original-data table
    // needed by gameplay progression without constructing SDL textures.
    bool loadGameplayOracle(const std::string& dataDir, std::string& error);
    bool loadSave(const std::string& path, PlayerSave& out, std::string& error) const;

    int terrainAt(int x, int y) const;
    SDL_Texture* terrainTexture(int logicalTile) const;
    SDL_Texture* animatedTerrainTexture(int logicalTile, std::uint32_t animationStep) const;
    SDL_Texture* physicalTexture(int physicalTile) const;
    SDL_Texture* magicProjectileTexture() const { return magicProjectileTex_; }
    SDL_Texture* titleTexture() const { return titleTex_; }
    SDL_Texture* launcherLogoTexture() const { return launcherLogoTex_; }
    const std::array<std::uint8_t,275*64>& launcherLogoMask() const { return launcherLogoMask_; }
    const std::array<std::uint8_t,8>& frontendCursorFrame(int frame) const { return frontendCursor_[static_cast<std::size_t>(frame&3)]; }
    const std::array<std::uint8_t,64>& titleFlagFrame(int frame) const { return titleFlagFrames_[static_cast<std::size_t>(frame%3)]; }

    const LocationInfo* locationAt(int worldX, int worldY) const;
    const std::vector<LocationInfo>& locations() const { return locations_; }

    int localMapIndexForLocation(const LocationInfo& location) const;
    void localEntryForLocation(const LocationInfo& location, int& x, int& y) const;
    std::uint8_t localRawTile(int mapIndex, int x, int y) const;
    int localDisplayTile(int mapIndex, int x, int y) const;
    bool localPassable(int mapIndex, int x, int y) const;
    SDL_Texture* localTexture(int physicalTile) const;
    // MONDAIN composite sprite slots: 0=blank, 1..19=EGAMOND.BIN entries 0..18.
    // Slot 20 is the separately staged EGATILES tile 10 player graphic.
    SDL_Texture* mondainTexture(int slot) const;
    const std::array<std::uint8_t,19*9>& mondainInitialArena() const { return mondainInitialArena_; }
    // M270: successful evil-gem destruction executes one exact source loop:
    // sound(2) five times. Both the ID and loop count are read from the user's
    // MONDAIN.EXE byte oracle. Final lethal damage uses normal sound(2);
    // the separately drawn victory popup then plays sound(5) exactly three times.
    int mondainGemDestroySoundId() const { return mondainGemDestroySoundId_; }
    int mondainGemDestroySoundCount() const { return mondainGemDestroySoundCount_; }
    // M26: NIF.BIN is the exact 320x168 1-bit victory bitmap.
    const std::array<std::uint8_t,6720>& nifBitmap() const { return nifBitmap_; }

    const std::array<LocalActor,15>& actorTemplate(int mapIndex) const;
    static const char* actorKindName(std::uint16_t tile, bool inTown);
    const std::string& princessNameForCastle(int locationId) const;
    static int weaponRange(int weaponIndex);
    static const char* weaponAttackName(int weaponIndex);

    // M10: exact 1987 OUT.EXE dungeon-monster vector streams.  The data is
    // parsed from the user's OUT.EXE at runtime; no original vector stream is
    // embedded in this clean-room source tree.
    const std::vector<DungeonVectorCommand>& dungeonMonsterVector(int monsterId) const;
    // M11: the same OUT.EXE vector pointer table continues with environment
    // shapes 25..29 (coffin/ladder pieces). Parsed at runtime from the user's executable.
    const std::vector<DungeonVectorCommand>& dungeonVector(int vectorId) const;
    int dungeonMonsterVectorBaseY(int depth) const;

    static constexpr int MAP_W=168, MAP_H=156;
    static constexpr int LOCAL_W=38, LOCAL_H=18, LOCAL_MAP_COUNT=10;
    static constexpr int LOCAL_ACTOR_COUNT=15;
    static constexpr int WORLD_OBJECT_COUNT=40;

private:
    bool loadOutOracle(const std::string& path, std::string& error);
    std::vector<std::uint8_t> map_;
    std::vector<std::uint8_t> tcd_;
    std::array<SDL_Texture*,52> tileTex_{};
    std::array<SDL_Texture*,4> waterAnimTex_{};
    SDL_Texture* titleTex_=nullptr;
    SDL_Texture* launcherLogoTex_=nullptr;
    std::array<std::uint8_t,275*64> launcherLogoMask_{};
    std::array<SDL_Texture*,51> townTex_{};
    std::array<SDL_Texture*,19> mondainTex_{};
    std::array<std::uint8_t,19*9> mondainInitialArena_{};
    int mondainGemDestroySoundId_=2;
    int mondainGemDestroySoundCount_=5;
    std::array<std::uint8_t,6720> nifBitmap_{};
    std::array<std::array<std::uint8_t,8>,4> frontendCursor_{};
    std::array<std::array<std::uint8_t,64>,3> titleFlagFrames_{};
    std::vector<LocationInfo> locations_;
    std::array<std::array<LocalActor,LOCAL_ACTOR_COUNT>,LOCAL_MAP_COUNT> actorTemplates_{};
    std::array<std::string,8> princessNames_{};
    std::array<std::vector<DungeonVectorCommand>,30> dungeonVectors_{};
    std::array<std::uint16_t,5> dungeonMonsterVectorBaseY_{};
    SDL_Texture* magicProjectileTex_{};
};
