#pragma once
// DJGPP is single-threaded for this backend. The original HF93 audio
// mutex requires a type, but native DOS audio will run synchronously.
#ifdef __DJGPP__
#include <mutex>
namespace std {
class mutex {
public:
 mutex() noexcept = default;
 ~mutex() = default;
 mutex(const mutex&) = delete;
 mutex& operator=(const mutex&) = delete;
 void lock() noexcept {}
 void unlock() noexcept {}
 bool try_lock() noexcept {return true;}
};
}
#endif