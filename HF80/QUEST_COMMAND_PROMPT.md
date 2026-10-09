# HF80 — Shift-Q command prompt lifecycle

Compared against the existing Zstats modal close handling in HF79.

Apply these changes to `HF79/src/main.cpp`:

1. In `if(shifted && k==SDLK_q)`, before opening `questPopup`, call `log.pushCommand("퀘스트 현황");`. `Log::pushCommand` marks the row as a command, and the renderer draws `>` on its left.
2. In `if(questPopup)`, change the Space handler to:
```cpp
if(k==SDLK_SPACE){
    questPopup=false;
    questPopupPresented=false;
    finishCommand(); // Zstats-style command completion and live > prompt
}
```
3. The existing modal input `스페이스바를 눌러 계속하세요:` remains visible only while `questPopup` is true. When dismissed it disappears and the standard `>` command cursor is drawn.

HF80 cross-compiled Windows `u1kr_clean` successfully. Windows package on authorized PC: `Ultima1_KR_Work/packages/HF80_QUEST_COMMAND_PROMPT.zip`.

In-game visual verification remains pending.
