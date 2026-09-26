#pragma once

enum class SfxEvent : int {
    OutdoorTerrainCollision = 0,

    LocalBlocked             = 1,
    InvalidXit               = 1,

    CombatHit                = 2,

    EnemyRangedLaunch        = 3,

    OutdoorLandMove          = 4,
    LocalPlayerMove          = 4,
    LocalActorMove           = 4,

    VehicleFire              = 6,

    PlayerAttackLaunch       = 7,
};

constexpr int sfxId(SfxEvent e) {
    return static_cast<int>(e);
}
