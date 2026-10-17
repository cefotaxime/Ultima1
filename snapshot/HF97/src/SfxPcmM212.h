#pragma once
#include <cstddef>
#include <cstdint>

// M216: exact signed-16-bit mono PCM extracted from the user-auditioned
// M212 preview WAVs (44.1 kHz). These are the approved reference buffers;
// runtime SFX must not re-synthesize or retime them.
namespace SfxPcmM212 {
struct Clip { const std::int16_t* data; std::size_t samples; };
constexpr int SAMPLE_RATE = 44100;
Clip clip(int soundId);
}