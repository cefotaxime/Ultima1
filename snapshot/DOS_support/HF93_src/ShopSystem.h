#pragma once

#include <array>
#include <string>
#include <vector>
#include "LocalSimulation.h"
#include "OriginalData.h"

enum class ShopKind { None, Armour, Food, Weapon, Magic, Pub, Transport };
enum class TradeStage { Idle, BuySell, Item, FoodQuantity, Message };

struct ShopOption {
    char key='?';
    int index=0;
    std::string name;
    int price=0;
    int quantity=0;
};

class ShopSystem {
public:
    bool begin(const OriginalData& original, const LocalState& local, const PlayerSave& player,
               std::vector<std::string>& messages);
    bool active() const { return stage_ != TradeStage::Idle; }
    TradeStage stage() const { return stage_; }
    ShopKind kind() const { return kind_; }

    // Returns true when this transaction command has finished and control should
    // return to the normal OUT command loop.
    bool handleKey(char key, const OriginalData& original, LocalState& local, PlayerSave& player,
                   LocalSimulation& simulation, std::vector<std::string>& messages);
    bool cancel(std::vector<std::string>& messages);

    // M149: expose the original transaction presentation state.  The DOS game
    // does not use the previous generic "Transact - kind / Coin" host popup;
    // main.cpp renders the 0x23C2 shop frame, town-specific sign and option rows.
    const std::vector<ShopOption>& options() const { return options_; }
    bool buying() const { return buying_; }
    const std::string& numberInput() const { return number_; }
    int townFamily() const { return townFamily_; }
    const std::vector<std::string>& popupMessage() const { return popupMessage_; }
    std::string inputLine() const;

    static int weaponBuyPrice(const PlayerSave& p,int index);
    static int weaponSellPrice(const PlayerSave& p,int index);
    static int armourBuyPrice(const PlayerSave& p,int index);
    static int armourSellPrice(const PlayerSave& p,int index);
    static int spellBuyPrice(const PlayerSave& p,int index);
    static int foodPackPrice(const PlayerSave& p);
    static int transportBuyPrice(const PlayerSave& p,int index);

private:
    void close(){ stage_=TradeStage::Idle; kind_=ShopKind::None; options_.clear(); number_.clear(); popupMessage_.clear(); popupPages_.clear(); pubPostDrinkPending_=false; }
    void buildOptions(const OriginalData& original, const LocalState& local, const PlayerSave& player);
    bool applyItem(char key, const OriginalData& original, const LocalState& local, PlayerSave& player, std::vector<std::string>& messages);
    static int worldObjectLast(const PlayerSave& player);
    static int worldTransportObjectCount(const PlayerSave& player);
    static int worldObjectAt(const PlayerSave& player,int x,int y);
    static std::array<int,3> nearbyTerrainCounts(const OriginalData& original,const PlayerSave& player);
    static bool spawnTransport(const OriginalData& original,PlayerSave& player,int transportIndex);
    void buyAle(LocalState& local, PlayerSave& player, LocalSimulation& simulation,
                std::vector<std::string>& messages);
    void queuePopupPage(std::vector<std::string> page);
    bool advancePopupPage();
    static ShopKind markerKind(std::uint8_t marker);
    static const char* kindName(ShopKind k);

    ShopKind kind_=ShopKind::None;
    TradeStage stage_=TradeStage::Idle;
    bool buying_=true;
    std::vector<ShopOption> options_;
    std::string number_;
    std::vector<std::string> popupMessage_;
    std::vector<std::vector<std::string>> popupPages_;
    bool pubPostDrinkPending_=false;
    int townFamily_=0; // local.mapIndex-2, OUT.EXE shop-title selector 0..7
};
