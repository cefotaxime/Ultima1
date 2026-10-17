from pathlib import Path
r=Path(r"C:\Users\hojun\Downloads\Ultima1_KR_Work")
m=(r/"dos_port/HF97_DOS_SOURCE/main.cpp").read_text(encoding="utf-8")
s=(r/"src/HF93/src/ShopSystem.cpp").read_text(encoding="utf-8")
tests={
"world_location_entry":"enterLocation(original,simulation,*loc,local,log);" in m,
"dungeon_entry_separate":"dungeonSystem.enter(dungeon,p,loc->id,m);" in m,
"town_trade_dispatch":"const bool opened=shop.begin(original,local,p,m);" in m,
"castle_king_dispatch":"const bool opened=king.begin(original,local,p,storeroomAllowance,m);" in m,
"trade_buysell_stage":"stage_=TradeStage::BuySell;" in s,
"food_vendor_marker":'if(m==0x38||m==0x39) return ShopKind::Food;' in s,
"weapon_buy_price":"int ShopSystem::weaponBuyPrice(" in s,
"weapon_sell_price":"int ShopSystem::weaponSellPrice(" in s,
"armour_buy_sell":"int ShopSystem::armourBuyPrice(" in s and "int ShopSystem::armourSellPrice(" in s,
"spell_buy_price":"int ShopSystem::spellBuyPrice(" in s,
"transport_buy_price":"int ShopSystem::transportBuyPrice(" in s,
"item_buy_apply":"p.coin=" in s,
}
for name,ok in tests.items():print(("PASS " if ok else "FAIL ")+name)
assert all(tests.values())