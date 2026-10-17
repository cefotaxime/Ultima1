#pragma once

#include <cstdint>

// Presentation-only hold whose clock does not start until the already-updated
// frame has actually been presented at least once.
//
// Intended sequence:
//   begin()                 -- immediate messages are already in the model/log
//   render + present frame
//   afterPresent(now)       -- visible hold starts here
//   ready(now, delayMs)     -- cannot become true before that presentation
class VisiblePresentationHold {
public:
    void begin(){
        pending_=true;
        started_=false;
        startedMs_=0;
    }

    void clear(){
        pending_=false;
        started_=false;
        startedMs_=0;
    }

    void afterPresent(std::uint32_t nowMs){
        if(pending_ && !started_){
            started_=true;
            startedMs_=nowMs;
        }
    }

    bool ready(std::uint32_t nowMs,std::uint32_t delayMs) const{
        return pending_ && started_ &&
            static_cast<std::uint32_t>(nowMs-startedMs_)>=delayMs;
    }

    bool pending() const { return pending_; }
    bool started() const { return started_; }
    std::uint32_t startedMs() const { return startedMs_; }

private:
    bool pending_=false;
    bool started_=false;
    std::uint32_t startedMs_=0;
};
