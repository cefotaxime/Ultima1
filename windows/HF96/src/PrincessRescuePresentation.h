#pragma once
#include <cstdint>

class PrincessRescuePresentation {
public:
    enum class Phase { None, AwaitFirstPresent, WaitingForDismiss };

    void begin(){
        phase_=Phase::AwaitFirstPresent;
        presentedAtMs_=0;
    }
    void clear(){
        phase_=Phase::None;
        presentedAtMs_=0;
    }
    bool active() const { return phase_!=Phase::None; }
    bool awaitingFirstPresent() const { return phase_==Phase::AwaitFirstPresent; }
    bool waitingForDismiss() const { return phase_==Phase::WaitingForDismiss; }

    void afterPresent(std::uint32_t nowMs){
        if(phase_==Phase::AwaitFirstPresent){
            phase_=Phase::WaitingForDismiss;
            presentedAtMs_=nowMs;
        }
    }

    bool canDismiss(std::uint32_t nowMs,std::uint32_t minimumHoldMs) const{
        return phase_==Phase::WaitingForDismiss &&
            static_cast<std::uint32_t>(nowMs-presentedAtMs_)>=minimumHoldMs;
    }

    std::uint32_t presentedAtMs() const { return presentedAtMs_; }

private:
    Phase phase_=Phase::None;
    std::uint32_t presentedAtMs_=0;
};