# HF79 — Quest repeat acceptance

HF79 updates the quest journal state tracking in HF78. Original gameplay permits a king quest to be accepted again after reward collection. The quest journal must retain its original list order and reset status from `완료` (complete) to `수락` (accepted) whenever `kingQuest[i]` transitions back to `0xFFFF`.

Apply to `HF78/src/main.cpp` within `QuestJournal::update`:

Replace:
```cpp
if(now==0xFFFF && state[i]==0){order.push_back(i);state[i]=1;changed=true;}
```
with:
```cpp
if(now==0xFFFF){
    // A previously completed quest may be accepted again.
    // Preserve its first-acceptance order; reset only its status.
    if(state[i]==0)order.push_back(i);
    if(state[i]!=1){state[i]=1;changed=true;}
}
```

Verified: Windows MinGW target `u1kr_clean` compiles and links. Interactive gameplay verification is outstanding.
Local Windows package: `Ultima1_KR_Work/packages/HF79_QUEST_REACCEPT.zip`.
