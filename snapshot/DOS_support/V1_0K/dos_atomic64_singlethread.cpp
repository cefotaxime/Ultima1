#include <stdint.h>
#include <string.h>
// DJGPP i486 single-thread adapter for HF93 std::atomic<uint64_t>.
// Do not use if interrupt handlers or true concurrency touch the same values.
extern "C" unsigned long long __atomic_load_8(const volatile void* p,int){
 unsigned long long v;memcpy(&v,(const void*)p,8);return v;
}
extern "C" void __atomic_store_8(volatile void* p,unsigned long long v,int){
 memcpy((void*)p,&v,8);
}
extern "C" unsigned long long __atomic_fetch_add_8(volatile void* p,unsigned long long x,int){
 unsigned long long old;memcpy(&old,(const void*)p,8);
 unsigned long long next=old+x;memcpy((void*)p,&next,8);return old;
}
