#pragma once
#include <string>
#include <vector>
#include "OriginalData.h"

class VehicleSystem {
public:
    // OUT.EXE Board command. Returns true when the command is fully consumed.
    bool board(PlayerSave& p, bool localActive, std::vector<std::string>& messages) const;
    // OUT.EXE X-it command. Returns true when the command is fully consumed.
    bool exitCraft(const OriginalData& original, PlayerSave& p, bool localActive,
                   std::vector<std::string>& messages) const;

    // Returns true if movement is permitted for the current transport.
    bool mayMove(const OriginalData& original, const PlayerSave& p, int nx, int ny,
                 std::vector<std::string>& messages) const;

    static const char* transportName(int transport);
    static int transportFromTile(int tile);

private:
    static int objectAt(const PlayerSave& p, int x, int y);
    static int lastObject(const PlayerSave& p);
    static void eraseObject(PlayerSave& p, int index);
    static bool appendObject(PlayerSave& p, const WorldObject& o);
    static bool anyLandWithin2(const OriginalData& original, int x, int y);
};