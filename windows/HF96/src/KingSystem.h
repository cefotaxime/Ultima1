#pragma once
#include <string>
#include <vector>
#include "OriginalData.h"
#include "LocalSimulation.h"

enum class KingStage { Idle, Choice, PenceAmount, ServicePopup };

class KingSystem {
public:
    bool begin(const OriginalData& original, const LocalState& local, PlayerSave& player,
               int& storeroomAllowance, std::vector<std::string>& messages);
    bool active() const { return stage_ != KingStage::Idle; }
    bool handleKey(char key, PlayerSave& player, std::vector<std::string>& messages);
    bool cancel(std::vector<std::string>& messages);
    KingStage stage() const { return stage_; }
    std::string inputLine() const;
    const std::vector<std::string>& popupLines() const { return popupLines_; }
    bool rewardPopup() const { return rewardPopup_; }

    // OUT.EXE landmark handler 0x52C6 -> quest completion helper 0x5285.
    static bool visitLandmark(int locationId, PlayerSave& player, int& lastLandmarkIndex,
                              std::vector<std::string>& messages, int* rewardSfxPending=nullptr);

private:
    void close(){ stage_=KingStage::Idle; number_.clear(); popupLines_.clear(); rewardPopup_=false; castleIndex_=0; }
    void rewardCompleted(PlayerSave& player, int& storeroomAllowance, std::vector<std::string>& messages);
    bool assignServiceQuest(PlayerSave& player, std::vector<std::string>& messages);
    static int quadrant(const PlayerSave& player);

    KingStage stage_=KingStage::Idle;
    int castleIndex_=0; // 1..8, locationId-32
    std::string number_;
    std::vector<std::string> popupLines_;
    bool rewardPopup_=false;
};