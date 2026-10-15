#pragma once

// Presentation-only two-stage state for ambient Bard/Jester dialogue.
//
// Begin after gameplay/RNG has already generated the delayed dialogue:
//   AwaitImmediatePresent -- the player's command/result must be presented first
//   AwaitDialoguePresent  -- the dialogue has been queued; it must be presented
//   None                  -- both visible holds are complete; input may resume
class LocalActorDialoguePresentation {
public:
    enum class Phase { None, AwaitImmediatePresent, AwaitDialoguePresent };

    void begin(){ phase_=Phase::AwaitImmediatePresent; }
    void clear(){ phase_=Phase::None; }
    bool active() const { return phase_!=Phase::None; }
    bool awaitingImmediatePresent() const { return phase_==Phase::AwaitImmediatePresent; }
    bool awaitingDialoguePresent() const { return phase_==Phase::AwaitDialoguePresent; }
    void dialogueQueued(){ phase_=Phase::AwaitDialoguePresent; }
    void finish(){ phase_=Phase::None; }
    Phase phase() const { return phase_; }

private:
    Phase phase_=Phase::None;
};