cmake_minimum_required(VERSION 3.16)
project(ultima1_kr_clean_m254 LANGUAGES C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# SDL2 discovery. Keep the historical pkg-config path on Linux/WSL so every
# legacy smoke target receives SDL2_INCLUDE_DIRS exactly as before. Native
# Windows uses the vcpkg/official CMake package and imported targets.
if(WIN32)
    find_package(SDL2 CONFIG REQUIRED)
    set(SDL2_INCLUDE_DIRS "")
    set(SDL2_CFLAGS_OTHER "")
    set(SDL2_LIBRARY_DIRS "")
    if(TARGET SDL2::SDL2main)
        set(SDL2_LIBRARIES SDL2::SDL2main SDL2::SDL2)
    else()
        set(SDL2_LIBRARIES SDL2::SDL2)
    endif()
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(SDL2 REQUIRED sdl2)
    # M238 canonical WSL/Linux path: talk to WSLg PulseAudio directly so SDL's
    # Pulse wait/mix scheduling cannot add another buffering layer.
    pkg_check_modules(PULSE REQUIRED libpulse)
endif()
add_executable(u1kr_clean
    src/main.cpp
    src/SfxSystem.cpp
    src/Apple1986Live.cpp
    third_party/fake6502/fake6502.c
    src/PcSpeakerEmulator.cpp
    src/SfxPcmOracleM225.cpp
    src/PixelFont.cpp
    src/FrontendSystem.cpp
    src/OriginalData.cpp
    src/LocalSimulation.cpp
    src/ShopSystem.cpp
    src/KingSystem.cpp
    src/VehicleSystem.cpp
    src/OverworldSimulation.cpp
    src/ResurrectionSystem.cpp
    src/DungeonSystem.cpp
    src/SpaceSystem.cpp
    src/CommandSystem.cpp
    src/MondainSystem.cpp
)
target_include_directories(u1kr_clean PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_clean PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_clean PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_clean PRIVATE ${SDL2_LIBRARIES})
if(NOT WIN32)
    target_compile_definitions(u1kr_clean PRIVATE U1_HAVE_LIBPULSE=1)
    target_include_directories(u1kr_clean PRIVATE ${PULSE_INCLUDE_DIRS})
    target_compile_options(u1kr_clean PRIVATE ${PULSE_CFLAGS_OTHER})
    target_link_directories(u1kr_clean PRIVATE ${PULSE_LIBRARY_DIRS})
    target_link_libraries(u1kr_clean PRIVATE ${PULSE_LIBRARIES})
endif()
if(WIN32)
    # IAudioClient3 is a Windows 10+ API.  ole32/uuid provide COM/WASAPI GUIDs;
    # avrt provides MMCSS scheduling for the event-driven low-latency thread.
    target_compile_definitions(u1kr_clean PRIVATE _WIN32_WINNT=0x0A00 WINVER=0x0A00)
    target_link_libraries(u1kr_clean PRIVATE ole32 uuid avrt ksguid)
endif()

# M39 fresh-save deterministic headless gameplay-progression smoke test. It loads only the
# original data tables needed for campaign logic and drives public gameplay APIs
# from the four gem quests through the NIF victory state.
add_executable(u1kr_campaign_smoke
    tools/campaign_smoke_m39.cpp
    src/PixelFont.cpp
    src/OriginalData.cpp
    src/LocalSimulation.cpp
    src/ShopSystem.cpp
    src/KingSystem.cpp
    src/VehicleSystem.cpp
    src/OverworldSimulation.cpp
    src/CommandSystem.cpp
    src/DungeonSystem.cpp
    src/SpaceSystem.cpp
    src/MondainSystem.cpp
)
target_include_directories(u1kr_campaign_smoke PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_campaign_smoke PRIVATE ${SDL2_CFLAGS_OTHER})
target_compile_definitions(u1kr_campaign_smoke PRIVATE U1_CAMPAIGN_DUNGEON_TIME_HASH=3)
target_link_directories(u1kr_campaign_smoke PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_campaign_smoke PRIVATE ${SDL2_LIBRARIES})

# M40 early-game Fighter steal/save-reload regression.
add_executable(u1kr_steal_route_m40
    tools/steal_route_m40.cpp
    src/PixelFont.cpp
    src/OriginalData.cpp
    src/LocalSimulation.cpp
    src/ShopSystem.cpp
    src/KingSystem.cpp
    src/VehicleSystem.cpp
    src/OverworldSimulation.cpp
    src/CommandSystem.cpp
    src/DungeonSystem.cpp
    src/SpaceSystem.cpp
    src/MondainSystem.cpp
)
target_include_directories(u1kr_steal_route_m40 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_steal_route_m40 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_steal_route_m40 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_steal_route_m40 PRIVATE ${SDL2_LIBRARIES})


# M80 standalone OUT.EXE/EGATILES animation inspector.
# This is deliberately separate from gameplay: it visualizes the exact
# overworld animation groups and lets the user pause/step them frame-by-frame.
add_executable(u1kr_tile_inspector
    tools/tile_animation_inspector.cpp
    src/PixelFont.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_tile_inspector PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_tile_inspector PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_tile_inspector PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_tile_inspector PRIVATE ${SDL2_LIBRARIES})


# M87 deterministic castle-Jester random-song regression.
add_executable(u1kr_jester_smoke_m87
    tools/jester_song_smoke_m87.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_jester_smoke_m87 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_jester_smoke_m87 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_jester_smoke_m87 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_jester_smoke_m87 PRIVATE ${SDL2_LIBRARIES})


# M93 reusable Korean final-consonant (받침) utility regression.
add_executable(u1kr_korean_particle_test_m94
    tools/hangul_batchim_test_m93.cpp
)
target_include_directories(u1kr_korean_particle_test_m94 PRIVATE src)


# M95 castle-local Attack/Guard wording and weapon-particle regression.
add_executable(u1kr_castle_combat_test_m95
    tools/castle_combat_test_m95.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_castle_combat_test_m95 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_castle_combat_test_m95 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_castle_combat_test_m95 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_castle_combat_test_m95 PRIVATE ${SDL2_LIBRARIES})


# M96 user-approved local NPC names + Killed! regression.
add_executable(u1kr_actor_translation_test_m96
    tools/actor_translation_test_m96.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_actor_translation_test_m96 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_actor_translation_test_m96 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_actor_translation_test_m96 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_actor_translation_test_m96 PRIVATE ${SDL2_LIBRARIES})


# M97 original Ready W/A/S popup state regression.
add_executable(u1kr_ready_popup_test_m97
    tools/ready_popup_test_m97.cpp
    src/CommandSystem.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_ready_popup_test_m97 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_ready_popup_test_m97 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_ready_popup_test_m97 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_ready_popup_test_m97 PRIVATE ${SDL2_LIBRARIES})


# M99 user-approved Ready prompts / armour names / Bow & Arrows correction.
add_executable(u1kr_ready_translation_test_m99
    tools/ready_translation_test_m99.cpp
    src/CommandSystem.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_ready_translation_test_m99 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_ready_translation_test_m99 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_ready_translation_test_m99 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_ready_translation_test_m99 PRIVATE ${SDL2_LIBRARIES})


# M101 Steal command/result localization regression.
add_executable(u1kr_steal_translation_test_m101
    tools/steal_translation_test_m101.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_steal_translation_test_m101 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_steal_translation_test_m101 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_steal_translation_test_m101 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_steal_translation_test_m101 PRIVATE ${SDL2_LIBRARIES})


# M102 Steal command-line marker contract + Ready "안 한다" regression.
add_executable(u1kr_steal_ready_test_m102
    tools/steal_ready_test_m102.cpp
    src/CommandSystem.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_steal_ready_test_m102 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_steal_ready_test_m102 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_steal_ready_test_m102 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_steal_ready_test_m102 PRIVATE ${SDL2_LIBRARIES})


# M103 user-approved castle names + one-line entry text regression.
add_executable(u1kr_castle_entry_test_m103
    tools/castle_entry_test_m103.cpp
)
target_include_directories(u1kr_castle_entry_test_m103 PRIVATE src)


# M104 original Ztats/Inventory model regression.
add_executable(u1kr_ztats_model_test_m104
    tools/ztats_model_test_m104.cpp
)
target_include_directories(u1kr_ztats_model_test_m104 PRIVATE src ${SDL2_INCLUDE_DIRS})


# M107 original Quit save restriction + visible save-gap regression.
add_executable(u1kr_quit_system_test_m107
    tools/quit_system_test_m107.cpp
)
target_include_directories(u1kr_quit_system_test_m107 PRIVATE src)


# M108 castle-area A..Z tile replacement + Korean centering regression.
add_executable(u1kr_castle_labels_test_m108
    tools/castle_labels_test_m108.cpp
)
target_include_directories(u1kr_castle_labels_test_m108 PRIVATE src)

# M109 Attack direction-turn semantics / projectile presentation / castle wording.
add_executable(u1kr_attack_presentation_test_m109
    tools/attack_presentation_test_m109.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_attack_presentation_test_m109 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_attack_presentation_test_m109 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_attack_presentation_test_m109 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_attack_presentation_test_m109 PRIVATE ${SDL2_LIBRARIES})


# M111 generic all-weapon Attack phrase regression.
add_executable(u1kr_attack_translation_test_m111
    tools/attack_translation_test_m111.cpp
)
target_include_directories(u1kr_attack_translation_test_m111 PRIVATE src)


# M112 overworld combat/reward localization regression.
add_executable(u1kr_overworld_combat_translation_test_m112
    tools/overworld_combat_translation_test_m112.cpp
    src/OverworldSimulation.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_overworld_combat_translation_test_m112 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_overworld_combat_translation_test_m112 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_overworld_combat_translation_test_m112 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_overworld_combat_translation_test_m112 PRIVATE ${SDL2_LIBRARIES})


# M113 command-window UTF-8 width-aware wrapping regression.
add_executable(u1kr_gameplay_text_wrap_test_m113
    tools/gameplay_text_wrap_test_m113.cpp
)
target_include_directories(u1kr_gameplay_text_wrap_test_m113 PRIVATE src)


# M114 shortened overworld combat strings + inline reward regression.
add_executable(u1kr_overworld_combat_translation_test_m114
    tools/overworld_combat_translation_test_m114.cpp
    src/OverworldSimulation.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_overworld_combat_translation_test_m114 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_overworld_combat_translation_test_m114 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_overworld_combat_translation_test_m114 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_overworld_combat_translation_test_m114 PRIVATE ${SDL2_LIBRARIES})


# M115 four-line-conscious overworld combat stream + monster-name regression.
add_executable(u1kr_overworld_combat_stream_test_m115
    tools/overworld_combat_stream_test_m115.cpp
    src/OverworldSimulation.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_overworld_combat_stream_test_m115 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_overworld_combat_stream_test_m115 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_overworld_combat_stream_test_m115 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_overworld_combat_stream_test_m115 PRIVATE ${SDL2_LIBRARIES})


# M117 exact OUT.EXE B157 world-object frame selection regression.
add_executable(u1kr_world_tile_animation_test_m117
    tools/world_tile_animation_test_m117.cpp
)
target_include_directories(u1kr_world_tile_animation_test_m117 PRIVATE src)


# M118 exact OUT.EXE 0x83FC enemy projectile presentation regression.
add_executable(u1kr_enemy_projectile_test_m118
    tools/enemy_projectile_test_m118.cpp
    src/OverworldSimulation.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_enemy_projectile_test_m118 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_enemy_projectile_test_m118 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_enemy_projectile_test_m118 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_enemy_projectile_test_m118 PRIVATE ${SDL2_LIBRARIES})


# M119 field/castle compact combat-message layout regression.
add_executable(u1kr_combat_message_layout_test_m119
    tools/combat_message_layout_test_m119.cpp
    src/OverworldSimulation.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_combat_message_layout_test_m119 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_combat_message_layout_test_m119 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_combat_message_layout_test_m119 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_combat_message_layout_test_m119 PRIVATE ${SDL2_LIBRARIES})


# M121 Drop/Ready shared popup, two-space cursor gap, and "안 한다" regression.
add_executable(u1kr_drop_ready_popup_test_m121
    tools/drop_ready_popup_test_m121.cpp
    src/CommandSystem.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_drop_ready_popup_test_m121 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_drop_ready_popup_test_m121 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_drop_ready_popup_test_m121 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_drop_ready_popup_test_m121 PRIVATE ${SDL2_LIBRARIES})

# M122 castle command translations + original Drop Pence no-popup regression.
add_executable(u1kr_castle_commands_test_m122
    tools/castle_commands_test_m122.cpp
    src/CommandSystem.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_castle_commands_test_m122 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_castle_commands_test_m122 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_castle_commands_test_m122 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_castle_commands_test_m122 PRIVATE ${SDL2_LIBRARIES})

# M123 OUT.EXE 0x584B castle Inform/search location-name regression.
add_executable(u1kr_castle_inform_test_m123
    tools/castle_inform_test_m123.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_castle_inform_test_m123 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_castle_inform_test_m123 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_castle_inform_test_m123 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_castle_inform_test_m123 PRIVATE ${SDL2_LIBRARIES})


# M124 Bard/Jester random-dialogue presentation separation regression.
add_executable(u1kr_local_actor_dialogue_delay_test_m124
    tools/local_actor_dialogue_delay_test_m124.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_local_actor_dialogue_delay_test_m124 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_local_actor_dialogue_delay_test_m124 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_local_actor_dialogue_delay_test_m124 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_local_actor_dialogue_delay_test_m124 PRIVATE ${SDL2_LIBRARIES})


# M126 user-visible delay clock: it starts only after the immediate frame was presented.
add_executable(u1kr_visible_presentation_hold_test_m126
    tools/visible_presentation_hold_test_m126.cpp
)
target_include_directories(u1kr_visible_presentation_hold_test_m126 PRIVATE src)

# M127 two-sided Bard/Jester presentation state regression.
add_executable(u1kr_local_actor_dialogue_presentation_test_m127
    tools/local_actor_dialogue_presentation_test_m127.cpp
)
target_include_directories(u1kr_local_actor_dialogue_presentation_test_m127 PRIVATE src)


# M129 Princess rescue popup/name localization + castle Unlock translations.
add_executable(u1kr_princess_unlock_presentation_test_m129
    tools/princess_unlock_presentation_test_m129.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_princess_unlock_presentation_test_m129 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_princess_unlock_presentation_test_m129 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_princess_unlock_presentation_test_m129 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_princess_unlock_presentation_test_m129 PRIVATE ${SDL2_LIBRARIES})


# M131 King completion localization + OUT.EXE landmark reward/localization regression.
add_executable(u1kr_king_landmark_localization_test_m131
    tools/king_landmark_localization_test_m131.cpp
    src/KingSystem.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_king_landmark_localization_test_m131 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_king_landmark_localization_test_m131 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_king_landmark_localization_test_m131 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_king_landmark_localization_test_m131 PRIVATE ${SDL2_LIBRARIES})

# M132 completed-quest king reward popup state regression.
add_executable(u1kr_king_reward_popup_test_m132
    tools/king_reward_popup_test_m132.cpp
    src/KingSystem.cpp
)
target_include_directories(u1kr_king_reward_popup_test_m132 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_king_reward_popup_test_m132 PRIVATE ${SDL2_CFLAGS_OTHER})
# M133 user-approved quest/gem/transport names + Ztats mapping regression.
add_executable(u1kr_localization_test_m133
    tools/localization_test_m133.cpp
)
target_include_directories(u1kr_localization_test_m133 PRIVATE ${SDL2_INCLUDE_DIRS} src)


# M135 object-aware overworld movement / water-craft boarding regression.
# This target supplies a focused terrainAt stub, so it intentionally does not
# link OriginalData.cpp or load any proprietary game data.
add_executable(u1kr_vehicle_board_access_test_m135
    tools/vehicle_board_access_test_m135.cpp
    src/VehicleSystem.cpp
)
target_include_directories(u1kr_vehicle_board_access_test_m135 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_vehicle_board_access_test_m135 PRIVATE ${SDL2_CFLAGS_OTHER})

# M136 user-approved transport/movement localization and Frigate facing enhancement.
add_executable(u1kr_frigate_direction_test_m136
    tools/frigate_direction_test_m136.cpp
)
target_include_directories(u1kr_frigate_direction_test_m136 PRIVATE src)

add_executable(u1kr_vehicle_localization_test_m136
    tools/vehicle_localization_test_m136.cpp
    src/VehicleSystem.cpp
)
target_include_directories(u1kr_vehicle_localization_test_m136 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_vehicle_localization_test_m136 PRIVATE ${SDL2_CFLAGS_OTHER})

# M219: X-it must use the original object-aware current-cell query so a craft
# cannot be dropped onto another live world object/transport.
add_executable(u1kr_vehicle_xit_occupied_cell_test_m219
    tools/vehicle_xit_occupied_cell_test_m219.cpp
    src/VehicleSystem.cpp
)
target_include_directories(u1kr_vehicle_xit_occupied_cell_test_m219 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_vehicle_xit_occupied_cell_test_m219 PRIVATE ${SDL2_CFLAGS_OTHER})


# M137: Frigate facing follows horizontal input before movement validation.
add_executable(u1kr_frigate_direction_test_m137
    tools/frigate_direction_test_m137.cpp
)
target_include_directories(u1kr_frigate_direction_test_m137 PRIVATE src)

# M140 fixed 8-direction overworld enemy projectile regression.
add_executable(u1kr_enemy_projectile_direction_test_m140
    tools/enemy_projectile_direction_test_m140.cpp
    src/OriginalData.cpp
    src/LocalSimulation.cpp
    src/OverworldSimulation.cpp
)
target_include_directories(u1kr_enemy_projectile_direction_test_m140 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_enemy_projectile_direction_test_m140 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_enemy_projectile_direction_test_m140 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_enemy_projectile_direction_test_m140 PRIVATE ${SDL2_LIBRARIES})


# M141 overworld Magic Missile / Kill projectile regression.
add_executable(u1kr_world_cast_test_m141
    tools/world_cast_attack_test_m141.cpp
    src/OverworldSimulation.cpp
    src/LocalSimulation.cpp
    src/OriginalData.cpp
)
target_include_directories(u1kr_world_cast_test_m141 PRIVATE ${SDL2_INCLUDE_DIRS} src)
target_compile_options(u1kr_world_cast_test_m141 PRIVATE ${SDL2_CFLAGS_OTHER})
target_link_directories(u1kr_world_cast_test_m141 PRIVATE ${SDL2_LIBRARY_DIRS})
target_link_libraries(u1kr_world_cast_test_m141 PRIVATE ${SDL2_LIBRARIES})

# M235 source-level PC-speaker impulse renderer smoke test.
add_executable(u1kr_pc_speaker_test_m230
    tools/pc_speaker_test_m230.cpp
    src/PcSpeakerEmulator.cpp
    src/SfxPcmOracleM225.cpp
)
target_include_directories(u1kr_pc_speaker_test_m230 PRIVATE src)

add_executable(u1kr_vehicle_facing_test_m268
    tools/vehicle_facing_test_m268.cpp
)
target_include_directories(u1kr_vehicle_facing_test_m268 PRIVATE src)