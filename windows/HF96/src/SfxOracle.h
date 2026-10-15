#pragma once

// M225: central original sound-event map.
//
// Rule: gameplay code names the original event, never a guessed numeric ID.
// Multiple events intentionally share one ID where OUT.EXE does so.  Timing
// and pitch calibration are deferred; this file captures only event identity.
enum class SfxEvent : int {
    OutdoorTerrainCollision = 0,
    SpaceCrunch              = 0,

    LocalBlocked             = 1,
    InvalidXit               = 1,
    InvalidAttackWeapon      = 1,

    CombatHit                = 2,
    TrapTriggered             = 2,
    SpacePlayerHit            = 2,
    SpaceAlienHit             = 2,

    EnemyRangedLaunch        = 3,

    OutdoorLandMove          = 4,
    LocalPlayerMove          = 4,
    LocalActorMove           = 4,
    FinalArenaPlayerMove     = 4,

    PositiveEffect           = 5,
    ResurrectionComplete     = 5,
    NoEffect                 = 6,

    PlayerAttackLaunch       = 7,
    LocalGuardAttack         = 7,
    FinalArenaPlayerAttack   = 7,
    FinalArenaEnemyAttack    = 3,
    VehicleFire              = 8,
    SpaceAlienFire           = 8,
    SpaceFlightPulse         = 9,
};

constexpr int sfxId(SfxEvent e) {
    return static_cast<int>(e);
}