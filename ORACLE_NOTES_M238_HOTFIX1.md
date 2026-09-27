# M238 hotfix1 — WSL build fix

M238 introduced std::unique_ptr<PulseAudioState> as an opaque implementation member while keeping SfxSystem() = default inline in the header. GCC instantiates member cleanup for that inline constructor in translation units where PulseAudioState is still incomplete, tripping std::default_delete's completeness check.

Hotfix1 changes no audio synthesis or transport behavior. It declares SfxSystem(); in the header and defines SfxSystem::SfxSystem() = default; in SfxSystem.cpp after PulseAudioState/WinAudioState are fully defined.
