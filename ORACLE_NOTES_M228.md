# M228 — queued-audio backend 복귀 + unplayed-tail remix

M227의 callback audio device는 사용자 WSLg/SDL 환경에서 무음 회귀를 일으켜 폐기한다. M228은 M225에서 실제 출력이 확인된 queued-audio backend로 복귀한다.

단, M225의 complete-WAV FIFO semantics는 복귀시키지 않는다. 새 source sound가 도착하면 `SDL_GetQueuedAudioSize()`로 아직 host device에 넘겨지지 않은 suffix 길이를 얻고, `queuedMix_` shadow에서 그 suffix를 보존한다. 새 capture PCM은 앞선 source call의 남은 `logicalDurationMs()` offset에서 sample-wise mix한다. 그 뒤 `SDL_ClearQueuedAudio()`로 SDL의 아직 재생되지 않은 queue만 교체하고, 보존+mix한 buffer를 `SDL_QueueAudio()`로 다시 넣는다.

따라서 clear는 tail 삭제가 아니라 **보존+mix한 queue로 교체**하기 위한 동작이다. sound(4)는 source 기준 약 5 ms 간격으로 이어지며 긴 impulse/filter tail은 overlap된다.

M227의 one-command-per-present 및 source logical busy gate는 유지한다. audible PCM tail은 입력 hold에 포함하지 않는다.
