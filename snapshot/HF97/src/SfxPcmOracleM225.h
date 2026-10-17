#pragma once
#include <cstddef>
#include <cstdint>

// M225: 48-kHz mono signed-16-bit presentation PCM extracted directly from
// the user's DOSBox Staging 0.82.2 captures.  These buffers retain the
// speaker/filter tail; logical DOS sound() blocking time is stored separately
// in SfxSystem and must not be inferred from clip length.
namespace SfxPcmOracleM225 {
struct Clip { const std::int16_t* data; std::size_t samples; };
constexpr int SAMPLE_RATE = 48000;
Clip clip(int soundId);
}